#include "SceneRenderer.hpp"
#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/AssetManager.hpp"
#include "scene/Components.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanPipeline.hpp"

void DefaultSceneRenderer::Init(VulkanRenderer* m_Backend, AssetManager* assetManager)
{
    // 1. Call the base class to store the pointers!
    SceneRenderer::Init(m_Backend, assetManager);
    
    // 2. Do any default renderer specific setup here...
    assetManager->AddMesh("Cube", std::make_unique<Mesh>(m_Backend->getDevice(), "Cube", cubeVertices, cubeIndices));
    assetManager->AddMesh("Square", std::make_unique<Mesh>(m_Backend->getDevice(), "Square", squareVertices, squareIndices));

    MaterialRenderState skyboxState;
    skyboxState.ShaderPath = "shaders/skybox.spv";
    // We are inside the cube, so we cull the front faces instead of the back!
    skyboxState.Culling = CullMode::None;
    // We disable depth writing so the skybox stays perfectly in the background
    skyboxState.Depth = DepthState::ReadOnly;

    m_SkyboxMaterial = assetManager->CreateMaterial(
        *m_Backend,
        "Builtin_Skybox",
        skyboxState,
        m_Backend->GetSwapchainFormat(),
        INVALID_ASSET_HANDLE // We pass the actual cubemap dynamically per-frame!
    );
}

RenderView DefaultSceneRenderer::ExtractRenderData(Scene& scene)
{
    RenderView view;

    // ==========================================
    // 1. EXTRACT CAMERA DATA (Global Frame Data)
    // ==========================================
    auto cameraView = scene.m_Registry.view<TransformComponent, CameraComponent>();
    for (auto entity : cameraView) {
        auto& transform = cameraView.get<TransformComponent>(entity);
        auto& camComp = cameraView.get<CameraComponent>(entity);

        if (camComp.Primary) {
            camComp.SceneCamera.RecalculateViewMatrix(transform.WorldMatrix);

            // Fill the byte-ready struct that will be memcpy'd into the UBO!
            view.FrameData.ViewMatrix = camComp.SceneCamera.GetViewMatrix();
            view.FrameData.ProjMatrix = camComp.SceneCamera.GetProjectionMatrix();
            break;
        }
    }

    // ==========================================
    // 2. EXTRACT OPAQUE MESHES (The Bucket Sort)
    // ==========================================
    auto meshView = scene.m_Registry.view<MeshComponent, TransformComponent>();
    for (auto e : meshView) {
		Entity entity{ e, &scene};
        auto& meshComp = entity.GetComponent<MeshComponent>();
        auto& transformComp = entity.GetComponent<TransformComponent>();

        if (meshComp.MeshHandle != INVALID_ASSET_HANDLE) {
            uint32_t texIndex = 0;
            VulkanPipeline* targetPipeline = nullptr;

            // Resolve the Material and Pipeline
            if (entity.HasComponent<MaterialComponent>()) {
                auto& matComp = entity.GetComponent<MaterialComponent>();
                if (matComp.MaterialHandle != INVALID_ASSET_HANDLE) {
                    Material* material = m_AssetManager->GetMaterial(matComp.MaterialHandle);
                    if (material) {
                        targetPipeline = material->GetPipeline();

                        // Resolve the Texture Index for the Bindless Array
                        if (material->GetTextureHandle() != INVALID_ASSET_HANDLE) {
                            Texture* albedo = m_AssetManager->GetTexture(material->GetTextureHandle());
                            if (albedo) texIndex = albedo->GetBindlessIndex();
                        }
                    }
                }
            }

            // Drop into our highly optimized sorting bucket!
            if (targetPipeline) {
                view.OpaqueBucket[targetPipeline][texIndex].push_back({
                    meshComp.MeshHandle,
                    transformComp.WorldMatrix
                    });
            }
        }
    }

    // ==========================================
    // 3. EXTRACT ENVIRONMENT
    // ==========================================
    auto skyboxView = scene.m_Registry.view<SkyboxComponent>();
    for (auto e : skyboxView) {
		Entity entity{ e, &scene };
        view.ActiveSkybox = entity.GetComponent<SkyboxComponent>().CubemapHandle;
        break;
    }

    return view;
}

void DefaultSceneRenderer::RenderScene(Scene& scene)
{
    // 1. Convert ECS state into Render Data
    RenderView frameView = ExtractRenderData(scene);

    // 2. Start the m_Backend Frame (Handles swapchain acquisition & UBO upload)
    if (m_Backend->BeginFrame(frameView))
    {
        m_Graph.Clear();

        // ==========================================
        // 3. IMPORT PHYSICAL TEXTURES
        // ==========================================
        RGTextureHandle viewportColor = m_Graph.ImportTexture("ViewportColor", vk::Format::eR8G8B8A8Unorm,
            m_Backend->GetViewportColorImage(), m_Backend->GetViewportColorImageView());

        RGTextureHandle viewportResolve = m_Graph.ImportTexture("ViewportResolve", vk::Format::eR8G8B8A8Unorm,
            m_Backend->GetViewportResolveImage(), m_Backend->GetViewportResolveImageView());

        RGTextureHandle viewportDepth = m_Graph.ImportTexture("ViewportDepth", vk::Format::eD32Sfloat,
            m_Backend->GetViewportDepthImage(), m_Backend->GetViewportDepthImageView());

        RGTextureHandle swapchainImage = m_Graph.ImportTexture("Swapchain", m_Backend->GetSwapchainFormat(),
            m_Backend->GetCurrentSwapchainImage(), m_Backend->GetCurrentSwapchainImageView());

        // ==========================================
        // 4. BUILD PASS 1: OPAQUE SCENE
        // ==========================================
        m_Graph.AddPass("Opaque_Scene",
            [&](RGBuilder& builder) {
                // Setup Phase: Tell the graph what layouts we need
                builder.Write(viewportColor, vk::ImageLayout::eColorAttachmentOptimal);
                builder.Write(viewportResolve, vk::ImageLayout::eColorAttachmentOptimal);
                builder.Write(viewportDepth, vk::ImageLayout::eDepthAttachmentOptimal);
            },
            [&](RGCommandList& cmdList, const RenderView& view) {

                // ==========================================
                // 1. DRAW SKYBOX (Draw FIRST so opaque meshes render over it)
                // ==========================================
                if (view.ActiveSkybox != INVALID_ASSET_HANDLE) {
                    Material* skyboxMat = m_AssetManager->GetMaterial(m_SkyboxMaterial);
                    Texture* skyboxTex = m_AssetManager->GetTexture(view.ActiveSkybox);
                    Mesh* cubeMesh = m_AssetManager->GetMesh("Cube");

                    if (skyboxMat && skyboxTex && cubeMesh) {
                        cmdList.BindPipeline(skyboxMat->GetPipeline());

                        PushConstants pc{
                            .modelMatrix = glm::mat4(1.0f), // Skybox shader usually handles positioning via view matrix
                            .textureIndex = skyboxTex->GetBindlessIndex()
                        };

                        cmdList.PushConstants(skyboxMat->GetPipeline()->getPipelineLayout(), vk::ShaderStageFlagBits::eAllGraphics, 0, pc);
                        cmdList.DrawMesh(cubeMesh);
                    }
                }

                // ==========================================
                // 2. DRAW OPAQUE MESHES
                // ==========================================
                for (const auto& [pipeline, materials] : view.OpaqueBucket) {
                    cmdList.BindPipeline(pipeline);

                    for (const auto& [texIndex, draws] : materials) {
                        for (const auto& draw : draws) {

                            PushConstants pc{
                                .modelMatrix = draw.Transform,
                                .textureIndex = texIndex
                            };

                            cmdList.PushConstants(pipeline->getPipelineLayout(), vk::ShaderStageFlagBits::eAllGraphics, 0, pc);

                            Mesh* mesh = m_AssetManager->GetMesh(draw.MeshHandle);
                            if (mesh) {
                                cmdList.DrawMesh(mesh);
                            }
                        }
                    }
                }
            }
        );

        // ==========================================
        // 5. BUILD PASS 2: UI (IMGUI)
        // ==========================================
        m_Graph.AddPass("ImGui_Pass",
            [&](RGBuilder& builder) {
                // Read the resolved MSAA image, write to the final OS window
                builder.Read(viewportResolve, vk::ImageLayout::eShaderReadOnlyOptimal);
                builder.Write(swapchainImage, vk::ImageLayout::eColorAttachmentOptimal);
            },
            [&](RGCommandList& cmdList, const RenderView& view) {
                m_Backend->recordImGuiCommands(cmdList.GetRaw());
            }
        );

        // ==========================================
        // 6. SUBMIT AND PRESENT
        // ==========================================
        m_Backend->ExecuteRenderGraph(m_Graph, frameView);
        m_Backend->EndFrame();
        m_Backend->Present();
    }
}