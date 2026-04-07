#include "EditorUI.hpp"
#include "scene/Components.hpp"
#include "scene/SceneSerializer.hpp"
#include "FileDialogs.hpp"
#include "core/Application.hpp"

void EditorUI::Draw(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer, std::function<void(const std::string&)> onSceneOpen)
{
    BeginDockspace();

    // Pass the function down to the menu bar!
    DrawMenuBar(scene, assetManager, renderer, onSceneOpen);

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

void EditorUI::DrawToolbar(SceneState& currentState, std::function<void()> onPlay, std::function<void()> onStop)
{
    // Remove padding so the toolbar sits flush
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

    // Make the window background slightly transparent or invisible if you prefer
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

    // These flags ensure it's just a raw panel, not a movable window
    ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("##Toolbar", nullptr, toolbarFlags);

    // Calculate button size and center it dynamically
    float buttonSize = ImGui::GetWindowHeight() - 4.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (buttonSize * 0.5f));

    // Draw the correct button based on the state we passed in
    if (currentState == SceneState::Edit)
    {
        // We push a green color for the Play button!
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        if (ImGui::Button("PLAY", ImVec2(buttonSize * 2.5f, buttonSize)))
        {
            onPlay(); // Tell the EditorLayer to start the game!
        }
        ImGui::PopStyleColor();
    }
    else if (currentState == SceneState::Play)
    {
        // We push a red color for the Stop button!
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("STOP", ImVec2(buttonSize * 2.5f, buttonSize)))
        {
            onStop(); // Tell the EditorLayer to stop the game!
        }
        ImGui::PopStyleColor();
    }

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
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

void EditorUI::DrawMenuBar(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer, std::function<void(const std::string&)> onSceneOpen) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {

            if (ImGui::MenuItem("Open Scene...")) {
                std::string filepath = FileDialogs::OpenFile("Scene File (*.yaml)\0*.yaml\0");
                if (!filepath.empty()) {
                    onSceneOpen(filepath);
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
    m_ViewportSize = ImGui::GetContentRegionAvail();

	scene.OnViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));

    renderer.ResizeViewport(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));

    ImTextureID textureID = reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(renderer.GetViewportTextureID()));
    if (renderer.GetViewportTextureID()) {
        ImGui::Image(textureID, m_ViewportSize);
    }

    // Call Gizmos while we are still inside the Viewport Window!
    DrawGizmos(scene, viewportOffset, m_ViewportSize);

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