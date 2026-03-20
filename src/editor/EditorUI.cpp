#include "EditorUI.hpp"
#include "scene/Components.hpp"
#include "FileDialogs.hpp"


void EditorUI::Draw(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer)
{
    // ==========================================
    // 1. CREATE THE MAIN DOCKSPACE
    // ==========================================
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    // These flags make the window invisible, un-draggable, and un-resizable
    ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    // Remove all padding and borders so it sits perfectly flush with the OS window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Main Editor Dockspace", nullptr, dockspace_flags);
    ImGui::PopStyleVar(3); // Pop the styles immediately

    // Submit the DockSpace API!
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);


    if (m_CurrentScene != &scene) {
        m_CurrentScene = &scene;
        m_SceneHierarchyPanel.SetContext(m_CurrentScene);
    }

    m_SceneHierarchyPanel.OnImGuiRender(assetManager, renderer);
    m_ContentBrowserPanel.OnImGuiRender(assetManager, renderer);

    // ==========================================
    // THE NEW VIEWPORT PANEL
    // ==========================================
    // Remove padding so the 3D scene touches the edges of the window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Get the exact screen coordinates where the image starts (below the title bar)
    ImVec2 viewportOffset = ImGui::GetCursorPos();
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x > 0.0f && viewportSize.y > 0.0f) {
        // Find your primary camera and UPDATE its projection matrix with the new panel size!
        auto cameraViewReg = scene.m_Registry.view<CameraComponent>();
        for (auto entityID : cameraViewReg) {
            auto& camComp = cameraViewReg.get<CameraComponent>(entityID);
            if (camComp.Primary) {
                // We use your exact method here to fix the stretching!
                camComp.SceneCamera.SetViewportSize(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));
            }
        }
    }

    // 1. Resize the Vulkan offscreen memory if the user resizes the panel
    renderer.ResizeViewport(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));

    ImTextureID textureID = reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(renderer.GetViewportTextureID()));
    // 2. Draw the 3D Scene into the panel!
    if (renderer.GetViewportTextureID()) {
        ImGui::Image(textureID, viewportSize);
    }

    // ==========================================
    // IMGUIZMO LOGIK
    // ==========================================

    Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();

    if (ImGui::IsKeyPressed(ImGuiKey_Q)) m_GizmoType = -1;
    if (ImGui::IsKeyPressed(ImGuiKey_T)) m_GizmoType = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GizmoType = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_S)) m_GizmoType = ImGuizmo::SCALE;

    // Zuerst prüfen wir, ob überhaupt eine Entity ausgewählt ist und ein Transform hat
    if (selectedEntity && selectedEntity.HasComponent<TransformComponent>()) {

        ImGuizmo::SetOrthographic(false);
        
        // NEW: Draw inside this specific window, not the whole screen!
        ImGuizmo::SetDrawlist();

        // NEW: Calculate the exact bounding box of the image on the monitor
        ImVec2 windowPos = ImGui::GetWindowPos();
        float rectX = windowPos.x + viewportOffset.x;
        float rectY = windowPos.y + viewportOffset.y;
        ImGuizmo::SetRect(rectX, rectY, viewportSize.x, viewportSize.y);

        // 1. Primäre Kamera finden und Matrizen holen
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

            bool snap = ImGui::IsKeyDown(ImGuiKey_LeftCtrl); // Ist die linke STRG-Taste gedrückt?

            float snapValue = (m_GizmoType == ImGuizmo::ROTATE) ? 45.0f : 0.5f;
            float snapValues[3] = { snapValue, snapValue, snapValue };

            if (m_GizmoType != -1) {
                // 2. Den Gizmo zeichnen und manipulieren
                // Aktuell hartkodiert auf TRANSLATE. Wir bauen später Buttons dafür ein!
                ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                    (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
                    nullptr, snap ? snapValues : nullptr);

                // 3. Wenn der Gizmo bewegt wurde, die Matrix wieder in die Component speichern
                if (ImGuizmo::IsUsing()) {
                    glm::mat4 localTransform = transform;

                    // Convert the modified World Matrix back into a Local Matrix if it has a parent
                    if (selectedEntity.HasParent()) {
                        glm::mat4 parentWorld = selectedEntity.GetParent().GetComponent<TransformComponent>().WorldMatrix;
                        localTransform = glm::inverse(parentWorld) * transform;
                    }

                    glm::vec3 translation, rotationDegrees, scale;
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localTransform),
                        glm::value_ptr(translation),
                        glm::value_ptr(rotationDegrees),
                        glm::value_ptr(scale));

                    tc.Translation = translation;
                    tc.SetEulerAngles(glm::radians(rotationDegrees));
                    tc.Scale = scale;
                }
            }
        }
    }

    ImGui::End();
	ImGui::PopStyleVar();

    ImGui::End();
}