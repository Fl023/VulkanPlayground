#include "EditorUI.hpp"
#include "scene/Components.hpp"
#include "scene/SceneSerializer.hpp"
#include "FileDialogs.hpp"
#include "core/Application.hpp"

void EditorUI::Draw(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer)
{
    BeginDockspace();
    DrawMenuBar(scene, assetManager, renderer);

    // Sync Scene context
    if (m_CurrentScene != &scene) {
        m_CurrentScene = &scene;
        m_SceneHierarchyPanel.SetContext(m_CurrentScene);
    }

    // Draw external panels
    m_SceneHierarchyPanel.OnImGuiRender(assetManager, renderer);
    m_ContentBrowserPanel.OnImGuiRender(assetManager, renderer);

    // Draw the 3D view and Gizmos
    DrawViewport(scene, renderer);

    EndDockspace();
}

void EditorUI::BeginDockspace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Main Editor Dockspace", nullptr, dockspace_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

void EditorUI::EndDockspace()
{
    ImGui::End(); // Ends the "Main Editor Dockspace" window
}

void EditorUI::DrawMenuBar(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer)
{
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {

            if (ImGui::MenuItem("Open Scene...")) {
                std::string filepath = FileDialogs::OpenFile("Scene File (*.yaml)\0*.yaml\0");
                if (!filepath.empty()) {
                    // Using the safe App Scene Swap logic!
                    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();
                    SceneSerializer serializer(newScene.get(), &assetManager);
                    serializer.Deserialize(filepath, renderer);
                    Application::Get().LoadScene(newScene);
                }
            }

            if (ImGui::MenuItem("Merge Scene...")) {
                std::string filepath = FileDialogs::OpenFile("Scene File (*.yaml)\0*.yaml\0");
                if (!filepath.empty()) {
                    SceneSerializer serializer(&scene, &assetManager);
                    serializer.Deserialize(filepath, renderer);
                }
            }

            if (ImGui::MenuItem("Save Scene As...")) {
                std::string filepath = FileDialogs::SaveFile("Scene File (*.yaml)\0*.yaml\0");
                if (!filepath.empty()) {
                    SceneSerializer serializer(&scene, &assetManager);
                    serializer.Serialize(filepath);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                Application::Get().Close();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void EditorUI::DrawViewport(Scene& scene, VulkanRenderer& renderer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 viewportOffset = ImGui::GetCursorPos();
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x > 0.0f && viewportSize.y > 0.0f) {
        auto cameraViewReg = scene.m_Registry.view<CameraComponent>();
        for (auto entityID : cameraViewReg) {
            auto& camComp = cameraViewReg.get<CameraComponent>(entityID);
            if (camComp.Primary) {
                camComp.SceneCamera.SetViewportSize(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));
            }
        }
    }

    renderer.ResizeViewport(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));

    ImTextureID textureID = reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(renderer.GetViewportTextureID()));
    if (renderer.GetViewportTextureID()) {
        ImGui::Image(textureID, viewportSize);
    }

    // Call Gizmos while we are still inside the Viewport Window!
    DrawGizmos(scene, viewportOffset, viewportSize);

    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorUI::DrawGizmos(Scene& scene, ImVec2 viewportOffset, ImVec2 viewportSize)
{
    Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();

    // Hotkeys
    if (ImGui::IsKeyPressed(ImGuiKey_Q)) m_GizmoType = -1;
    if (ImGui::IsKeyPressed(ImGuiKey_T)) m_GizmoType = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GizmoType = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_S)) m_GizmoType = ImGuizmo::SCALE;

    if (selectedEntity && selectedEntity.HasComponent<TransformComponent>() && m_GizmoType != -1) {

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        ImVec2 windowPos = ImGui::GetWindowPos();
        float rectX = windowPos.x + viewportOffset.x;
        float rectY = windowPos.y + viewportOffset.y;
        ImGuizmo::SetRect(rectX, rectY, viewportSize.x, viewportSize.y);

        glm::mat4 cameraView = glm::mat4(1.0f);
        glm::mat4 cameraProjection = glm::mat4(1.0f);
        bool cameraFound = false;

        auto cameraViewReg = scene.m_Registry.view<CameraComponent, TransformComponent>();
        for (auto entityID : cameraViewReg) {
            auto& camComp = cameraViewReg.get<CameraComponent>(entityID);
            if (camComp.Primary) {
                cameraView = camComp.SceneCamera.GetViewMatrix();
                cameraProjection = camComp.SceneCamera.GetProjectionMatrix();
                cameraFound = true;
                break;
            }
        }

        if (cameraFound) {
            auto& tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.WorldMatrix;

            bool snap = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);
            float snapValue = (m_GizmoType == ImGuizmo::ROTATE) ? 45.0f : 0.5f;
            float snapValues[3] = { snapValue, snapValue, snapValue };

            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
                nullptr, snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing()) {
                glm::mat4 localTransform = transform;

                if (selectedEntity.HasParent()) {
                    glm::mat4 parentWorld = selectedEntity.GetParent().GetComponent<TransformComponent>().WorldMatrix;
                    localTransform = glm::inverse(parentWorld) * transform;
                }

                glm::vec3 translation, rotationDegrees, scale;
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localTransform),
                    glm::value_ptr(translation), glm::value_ptr(rotationDegrees), glm::value_ptr(scale));

                tc.Translation = translation;
                tc.SetEulerAngles(glm::radians(rotationDegrees));
                tc.Scale = scale;
            }
        }
    }
}