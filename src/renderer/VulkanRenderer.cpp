#include "VulkanRenderer.hpp"


VulkanRenderer::VulkanRenderer(VulkanWindow& window)
    : window(window),
      context(),
      device(context, window),
      swapChain(device, window)
{
    window.setUserPointer(this);

	createPipelines();

    vk::DeviceSize uboSize = sizeof(UniformBufferObject);

    const vk::raii::DescriptorSetLayout& cameraLayout = m_mainShader->getLayouts()[0];
    const vk::raii::DescriptorSetLayout& skyboxLayout = m_skyboxShader->getLayouts()[1];

    frames.reserve(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        frames.emplace_back(device, cameraLayout, skyboxLayout, uboSize);
    }

    createBindlessDescriptorSet();
    createDefaultTexture();

    createColorResources();
    createDepthResources();

	createSyncObjects();
    imGuiLayer = std::make_unique<ImGuiLayer>(device, swapChain);
    m_ViewportTarget.emplace(
        device,
        static_cast<uint32_t>(window.getWidth()),
        static_cast<uint32_t>(window.getHeight()),
        swapChain.getSurfaceFormat().format
    );
}

VulkanRenderer::~VulkanRenderer()
{
    try {
        device.getDevice().waitIdle();
    } catch (...) {
        std::cerr << "Error waiting for device idle in Renderer destructor." << std::endl;
    }
}

const VulkanContext& VulkanRenderer::getContext() const
{
    return context;
}

const VulkanWindow& VulkanRenderer::getWindow() const
{
    return window;
}

void VulkanRenderer::drawFrame(Scene& scene, AssetManager& assetManager)
{
    // 1. Wait for the previous frame to finish
    auto fenceResult = device.getDevice().waitForFences(*frames[frameIndex].inFlightFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to wait for fence!");
    }

    for (auto& func : m_DeletionQueues[frameIndex]) {
        func(); // Execute the lambda (this destroys the VRAM)
    }
    m_DeletionQueues[frameIndex].clear();

    // 2. Acquire an image from the swap chain
    vk::Semaphore acquireSemaphore = *frames[frameIndex].presentCompleteSemaphore;
    auto [result, imageIndex] = swapChain.getSwapChain().acquireNextImage(UINT64_MAX, acquireSemaphore, nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchainResources();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

	// Update uniform buffer with the current image index before recording commands
    updateUniformBuffer(frameIndex, scene);

    // Only reset the fence if we are actually going to submit work
    device.getDevice().resetFences(*frames[frameIndex].inFlightFence);

    // 3. Record commands
    frames[frameIndex].commandBuffer.reset();
    recordCommandBuffer(imageIndex, scene, assetManager);

    // 4. Submit the recorded command buffer
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    
    vk::Semaphore renderFinishedSemaphore = *renderFinishedSemaphores[imageIndex];
    vk::CommandBuffer cb = *frames[frameIndex].commandBuffer;

    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &acquireSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &cb,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinishedSemaphore 
    };

    device.getQueue().submit(submitInfo, *frames[frameIndex].inFlightFence);

    // 5. Present the swap chain image
    const vk::SwapchainKHR sc = *swapChain.getSwapChain();

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &sc,
        .pImageIndices = &imageIndex 
    };

    try {
        result = device.getQueue().presentKHR(presentInfoKHR);
    } catch (const vk::OutOfDateKHRError&) {
        result = vk::Result::eErrorOutOfDateKHR;
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapchainResources();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Advance frame index
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::beginUI()
{
    imGuiLayer->beginFrame();
}

void VulkanRenderer::AddTextureToBindlessArray(Texture* texture)
{
    uint32_t indexToUse;

    // 1. Check the free list first
    if (!m_FreeTextureIndices.empty()) {
        indexToUse = m_FreeTextureIndices.back();
        m_FreeTextureIndices.pop_back();
    }
    // 2. Otherwise, allocate a new one at the end
    else {
        if (currentTextureIndex >= MAX_BINDLESS_TEXTURES) {
            throw std::runtime_error("Exceeded maximum bindless textures!");
        }
        indexToUse = currentTextureIndex++;
    }

    // 3. Give the index to the Texture
    texture->SetBindlessIndex(indexToUse);

    // 4. Update the Vulkan array
    vk::DescriptorImageInfo imageInfo = texture->GetDescriptorInfo();
    vk::WriteDescriptorSet descriptorWrite{
        .dstSet = *bindlessDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = indexToUse,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo
    };

    device.getDevice().updateDescriptorSets(descriptorWrite, {});
}

void VulkanRenderer::FreeBindlessIndex(uint32_t index)
{
    m_FreeTextureIndices.push_back(index);
}

void VulkanRenderer::SubmitToDeletionQueue(std::function<void()>&& function)
{
    m_DeletionQueues[frameIndex].push_back(std::move(function));
}

void VulkanRenderer::createPipelines()
{
    m_mainShader = std::make_unique<MainGraphicsShader>(device, "shaders/shader.spv");

    PipelineConfigInfo config{};
    VulkanPipeline::defaultPipelineConfigInfo(config, device.getMsaaSamples());

    // Add your Push Constant range
    config.pushConstantRanges.push_back({
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(PushConstants)
        });

    // Set dynamic rendering formats based on the swapchain
    config.colorAttachmentFormats = { swapChain.getSurfaceFormat().format };
    config.depthAttachmentFormat = vk::Format::eD32Sfloat;

    m_graphicsPipeline = std::make_unique<VulkanPipeline>(device, *m_mainShader, config);



    m_skyboxShader = std::make_unique<SkyboxShader>(device, "shaders/skybox.spv");

    PipelineConfigInfo skyboxConfig{};
    VulkanPipeline::defaultPipelineConfigInfo(skyboxConfig, device.getMsaaSamples());

    // Override states specifically for the skybox
    skyboxConfig.rasterizationInfo.cullMode = vk::CullModeFlagBits::eNone;
    skyboxConfig.depthStencilInfo.depthWriteEnable = vk::False;
    skyboxConfig.depthStencilInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;

    skyboxConfig.colorAttachmentFormats = { swapChain.getSurfaceFormat().format };
    skyboxConfig.depthAttachmentFormat = vk::Format::eD32Sfloat;

    auto positionAttribute = skyboxConfig.attributeDescriptions[0];
    skyboxConfig.attributeDescriptions.clear();
    skyboxConfig.attributeDescriptions.push_back(positionAttribute);

    m_skyboxPipeline = std::make_unique<VulkanPipeline>(device, *m_skyboxShader, skyboxConfig);
}

void VulkanRenderer::createSyncObjects()
{
    renderFinishedSemaphores.clear();
    // Erstelle genau ein Semaphore pro physischem Swapchain-Bild
    for (size_t i = 0; i < swapChain.getImages().size(); i++) {
        renderFinishedSemaphores.emplace_back(device.getDevice(), vk::SemaphoreCreateInfo());
    }
}

void VulkanRenderer::createDepthResources()
{
    depthImage.emplace(device, 
        swapChain.getExtent().width, 
        swapChain.getExtent().height, 
        1,
        device.getMsaaSamples(),
        vk::Format::eD32Sfloat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eDepthStencilAttachment, 
        vk::MemoryPropertyFlagBits::eDeviceLocal, 
        vk::ImageAspectFlagBits::eDepth);
}

void VulkanRenderer::createColorResources()
{
    colorImage.emplace(device,
        swapChain.getExtent().width,
        swapChain.getExtent().height,
        1,
        device.getMsaaSamples(),
        swapChain.getSurfaceFormat().format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );
}

void VulkanRenderer::recreateSwapchainResources()
{
    device.getDevice().waitIdle();
    swapChain.recreate();
    createColorResources();
    createDepthResources();
    createSyncObjects();
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage, Scene& scene) {
    UniformBufferObject ubo{};

    auto view = scene.m_Registry.view<TransformComponent, CameraComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& camComp = view.get<CameraComponent>(entity);

        if (camComp.Primary) {
            // 1. Viewport an Swapchain anpassen
            //camComp.SceneCamera.SetViewportSize(swapChain.getExtent().width, swapChain.getExtent().height);

            // 2. View-Matrix basierend auf dem Transform der Entity berechnen
            camComp.SceneCamera.RecalculateViewMatrix(transform.WorldMatrix);

            // 3. Matrizen ins UBO kopieren
            ubo.view = camComp.SceneCamera.GetViewMatrix();
            ubo.proj = camComp.SceneCamera.GetProjectionMatrix();
            break;
        }
    }

    memcpy(frames[currentImage].uniformBuffer->getMappedData(), &ubo, sizeof(ubo));
}

void VulkanRenderer::recordCommandBuffer(uint32_t imageIndex, Scene& scene, AssetManager& assetManager) {
    auto& commandBuffer = frames[frameIndex].commandBuffer;
    
    // Begin command buffer
    vk::CommandBufferBeginInfo beginInfo{};
    commandBuffer.begin(beginInfo);
    
    // 1. Draw the 3D World to the Offscreen Viewport
	recordSceneCommands(commandBuffer, scene, assetManager);

    // 2. Draw ImGui (and the Viewport image) to the Swapchain Monitor
	recordUICommands(commandBuffer, imageIndex);

    commandBuffer.end();
}

void VulkanRenderer::recordSceneCommands(vk::raii::CommandBuffer &commandBuffer, Scene& scene, AssetManager& assetManager)
{
    // 1. Transition Viewport Targets
    device.transitionImageLayout(commandBuffer, *m_ViewportTarget->GetResolveImage().getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 1, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput, {}, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageAspectFlagBits::eColor);
    device.transitionImageLayout(commandBuffer, *m_ViewportTarget->GetColorImage().getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 1, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageAspectFlagBits::eColor);
    device.transitionImageLayout(commandBuffer, *m_ViewportTarget->GetDepthImage().getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, 1, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::ImageAspectFlagBits::eDepth);

    // 2. Dynamic Rendering Setup
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo colorAttachment = {
        .imageView = *m_ViewportTarget->GetColorImage().getImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = *m_ViewportTarget->GetResolveImage().getImageView(),
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingAttachmentInfo depthAttachment = {
        .imageView = *m_ViewportTarget->GetDepthImage().getImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea = { {0, 0}, {m_ViewportTarget->GetWidth(), m_ViewportTarget->GetHeight()} },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachment
    };

    commandBuffer.beginRendering(renderingInfo);

    // 3. Bind State
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline->getPipeline());
    std::array<vk::DescriptorSet, 2> setsToBind = { *frames[frameIndex].cameraSet, *bindlessDescriptorSet };
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline->getPipelineLayout(), 0, setsToBind, {});

    vk::Viewport viewport{
        0.0f,                                   // x
        (float)m_ViewportTarget->GetHeight(),   // y (Shift the origin to the bottom of the screen)
        (float)m_ViewportTarget->GetWidth(),    // width
        -(float)m_ViewportTarget->GetHeight(),  // height (Make the height negative to flip the rendering)
        0.0f,                                   // minDepth
        1.0f                                    // maxDepth
    };
    commandBuffer.setViewport(0, viewport);
    vk::Rect2D scissor{ {0, 0}, {m_ViewportTarget->GetWidth(), m_ViewportTarget->GetHeight()} };
    commandBuffer.setScissor(0, scissor);

    // 4. Draw Entities
    auto view = scene.m_Registry.view<MeshComponent, TransformComponent>();
    for (auto entity : view) {
        auto& meshComp = view.get<MeshComponent>(entity);
        auto& transformComp = view.get<TransformComponent>(entity);

        if (meshComp.MeshHandle != INVALID_ASSET_HANDLE) {
            Mesh* mesh = assetManager.GetMesh(meshComp.MeshHandle);
            if (mesh) {
                uint32_t texIndex = 0;
                if (scene.m_Registry.all_of<MaterialComponent>(entity)) {
                    auto& matComp = scene.m_Registry.get<MaterialComponent>(entity);
                    if (matComp.MaterialHandle != INVALID_ASSET_HANDLE) {
                        Material* material = assetManager.GetMaterial(matComp.MaterialHandle);
                        if (material && material->GetTextureHandle() != INVALID_ASSET_HANDLE) {
                            Texture* albedo = assetManager.GetTexture(material->GetTextureHandle());
                            if (albedo) texIndex = albedo->GetBindlessIndex();
                        }
                    }
                }

                PushConstants pushData{ .modelMatrix = transformComp.WorldMatrix, .textureIndex = texIndex };
                commandBuffer.pushConstants<PushConstants>(m_graphicsPipeline->getPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushData);
                commandBuffer.bindVertexBuffers(0, *mesh->getVertexBuffer(), { 0 });
                commandBuffer.bindIndexBuffer(*mesh->getIndexBuffer(), 0, vk::IndexType::eUint16);
                commandBuffer.drawIndexed(mesh->getIndexCount(), 1, 0, 0, 0);
            }
        }
    }

    // --- DRAW SKYBOX ---
    auto skyboxView = scene.m_Registry.view<SkyboxComponent>();
    for (auto entity : skyboxView) {
        auto& skybox = skyboxView.get<SkyboxComponent>(entity);

        if (skybox.CubemapHandle != INVALID_ASSET_HANDLE) {

            // 1. The Dirty Check (Only update if the texture changed!)
            if (frames[frameIndex].activeSkyboxHandle != skybox.CubemapHandle) {
                Texture* cubemap = assetManager.GetTexture(skybox.CubemapHandle);

                if (cubemap) {
                    vk::DescriptorImageInfo imageInfo = cubemap->GetDescriptorInfo();
                    vk::WriteDescriptorSet write{
                        .dstSet = *frames[frameIndex].skyboxSet,
                        .dstBinding = 0,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                        .pImageInfo = &imageInfo
                    };

                    device.getDevice().updateDescriptorSets(write, {});
                    frames[frameIndex].activeSkyboxHandle = skybox.CubemapHandle;
                }
            }

            // 2. Bind Pipeline & Descriptor Sets
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_skyboxPipeline->getPipeline());

            std::array<vk::DescriptorSet, 2> setsToBind = {
                *frames[frameIndex].cameraSet,
                *frames[frameIndex].skyboxSet
            };

            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                *m_skyboxPipeline->getPipelineLayout(),
                0, setsToBind, {}
            );

            // 3. Draw the Cube!
            Mesh* cubeMesh = assetManager.GetMesh("Cube");
            if (cubeMesh) {
                commandBuffer.bindVertexBuffers(0, *cubeMesh->getVertexBuffer(), { 0 });
                commandBuffer.bindIndexBuffer(*cubeMesh->getIndexBuffer(), 0, vk::IndexType::eUint16);
                commandBuffer.drawIndexed(cubeMesh->getIndexCount(), 1, 0, 0, 0);
            }
        }
    }

    // 5. End Scene Render
    commandBuffer.endRendering();
}

void VulkanRenderer::recordUICommands(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex)
{
    // 1. Transition Viewport Resolve Image for ImGui to read
    device.transitionImageLayout(commandBuffer, *m_ViewportTarget->GetResolveImage().getImage(), vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eShaderRead, vk::ImageAspectFlagBits::eColor);

    // 2. Transition Swapchain Image for Drawing
    device.transitionImageLayout(commandBuffer, swapChain.getImages()[imageIndex], vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 1, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageAspectFlagBits::eColor);

    // 3. Dynamic Rendering Setup for UI
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo swapchainAttachment{
        .imageView = swapChain.getImageViews()[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingInfo swapchainRenderInfo{
        .renderArea = { {0, 0}, swapChain.getExtent() },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &swapchainAttachment
    };

    commandBuffer.beginRendering(swapchainRenderInfo);

    // 4. Draw UI
    imGuiLayer->recordImGuiCommands(commandBuffer);

    commandBuffer.endRendering();

    // 5. Transition Swapchain to Present
    device.transitionImageLayout(commandBuffer, swapChain.getImages()[imageIndex], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, 1, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe, vk::AccessFlagBits2::eColorAttachmentWrite, {}, vk::ImageAspectFlagBits::eColor);
}

void VulkanRenderer::createBindlessDescriptorSet()
{
    // 1. Create a pool large enough for our single massive set
    vk::DescriptorPoolSize poolSize{
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = MAX_BINDLESS_TEXTURES
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        .maxSets = 1, // We only ever allocate ONE set
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    bindlessPool = vk::raii::DescriptorPool(device.getDevice(), poolInfo);

    uint32_t variableDescCounts[] = { MAX_BINDLESS_TEXTURES };

    vk::DescriptorSetVariableDescriptorCountAllocateInfo variableAllocInfo{
        .descriptorSetCount = 1,
        .pDescriptorCounts = variableDescCounts
    };

    vk::DescriptorSetLayout materialLayout = *m_mainShader->getLayouts()[1];

    // 2. Allocate the single global set
    vk::DescriptorSetAllocateInfo allocInfo{
        .pNext = &variableAllocInfo,
        .descriptorPool = *bindlessPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &materialLayout
    };

    bindlessDescriptorSet = std::move(device.getDevice().allocateDescriptorSets(allocInfo).front());
}

void VulkanRenderer::createDefaultTexture()
{
    // 1. Create a 1x1 white pixel in CPU memory (RGBA: 255, 255, 255, 255)
    uint32_t whitePixel = 0xFFFFFFFF;

    // 2. Construct the texture directly using our unique_ptr
    m_DefaultTexture = std::make_unique<Texture>(device, 1, 1, &whitePixel);

    // 3. Add it to the bindless array using the raw pointer
    // Since it is the very first texture, it gets Slot 0!
    AddTextureToBindlessArray(m_DefaultTexture.get());
}
