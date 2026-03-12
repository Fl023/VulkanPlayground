#include "EditorUI.hpp"
#include "scene/Components.hpp"
#include "FileDialogs.hpp"


void EditorUI::Draw(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer)
{
    if (m_CurrentScene != &scene) {
        m_CurrentScene = &scene;
        m_SceneHierarchyPanel.SetContext(m_CurrentScene);
    }

    m_SceneHierarchyPanel.OnImGuiRender(assetManager, renderer);
    m_ContentBrowserPanel.OnImGuiRender(assetManager, renderer);

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
        // Da wir noch keinen eigenen Viewport haben, zeichnen wir über den gesamten Bildschirm
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        // Die Größe des aktuellen ImGui-Fensters (bzw. Bildschirms)
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);

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

                // VULKAN HACK FÜR IMGUIZMO: 
                // Da Vulkan Y nach unten hat (und ImGuizmo OpenGL erwartet),
                // müssen wir den Y-Flip der Projection temporär rückgängig machen.
                cameraProjection[1][1] *= -1.0f;

                cameraFound = true;
                break;
            }
        }

        if (cameraFound) {
            auto& tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetTransform();

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
                    glm::vec3 translation, rotationDegrees, scale;
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform),
                        glm::value_ptr(translation),
                        glm::value_ptr(rotationDegrees),
                        glm::value_ptr(scale));

                    tc.Translation = translation;
                    // ImGuizmo gibt Rotation in Grad zurück, aber deine Component speichert Radiant
                    tc.Rotation = glm::radians(rotationDegrees);
                    tc.Scale = scale;
                }
            }
        }
    }
}