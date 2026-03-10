#include "EditorUI.hpp"
#include "scene/Components.hpp"
#include <imgui_stdlib.h>
#include "FileDialogs.hpp"

template<typename T, typename UIFunction>
static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
{
    if (!entity.HasComponent<T>()) return;

    bool removeComponent = false;

    ImGui::PushID(name.c_str());

    // ImGuiTreeNodeFlags_AllowItemOverlap ist wichtig, damit der Button auf der selben Zeile funktioniert
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap;
    bool open = ImGui::TreeNodeEx("##Node", flags, "%s", name.c_str());

    // Lösch-Button rechtsbündig
    ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
    if (ImGui::Button("X")) {
        removeComponent = true;
    }

    if (open) {
        auto& component = entity.GetComponent<T>();
        uiFunction(component);
        ImGui::TreePop();
    }

    if (removeComponent) {
        entity.RemoveComponent<T>();
    }

    ImGui::PopID();
}

void EditorUI::Draw(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer)
{
    ImGui::Begin("Entities of current Scene");

    if (ImGui::Button("Create Entity")) {
        scene.CreateEntity("New Entity");
    }

    ImGui::Separator();

    auto sceneView = scene.m_Registry.view<TagComponent>();

    // Temporärer Speicher für zu löschende Entities, damit wir nicht während der Iteration löschen
    Entity entityToDestroy{};

    for (auto entityID : sceneView) {
        Entity entity{ entityID, &scene };
        auto& tag = sceneView.get<TagComponent>(entity).Tag;

        ImGui::PushID((int)entityID);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (m_SelectedEntity == entity) {
            flags |= ImGuiTreeNodeFlags_Selected; // Hebt die aktuelle Entity farblich hervor
        }

        // --- GANZES ENTITY LÖSCHEN (Rechtsklick-Menü) ---
        bool entityDeleted = false;
        bool open = ImGui::TreeNodeEx((void*)(intptr_t)entityID, flags, "%s", tag.c_str());

        if (ImGui::IsItemClicked()) {
            m_SelectedEntity = entity;
        }

        if (ImGui::BeginPopupContextItem()) { // Rechtsklick auf den Knoten
            if (ImGui::MenuItem("Delete Entity")) {
                entityDeleted = true;
            }
            ImGui::EndPopup();
        }

        if (open) {

            // 1. TAG COMPONENT (Sollte man idR. nicht löschen können)
            if (ImGui::TreeNode("TagComponent")) {
                ImGui::InputText("Tag", &tag);
                ImGui::TreePop();
            }

            // 2. TRANSFORM COMPONENT
            DrawComponent<TransformComponent>("TransformComponent", entity, [](auto& tc) {
                ImGui::DragFloat3("Translation", glm::value_ptr(tc.Translation), 0.1f);

                glm::vec3 rotationDegrees = glm::degrees(tc.Rotation);
                if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotationDegrees), 1.0f))
                    tc.Rotation = glm::radians(rotationDegrees);

                ImGui::DragFloat3("Scale", glm::value_ptr(tc.Scale), 0.1f, 0.0f);
                });

            // 3. MESH COMPONENT
            DrawComponent<MeshComponent>("MeshComponent", entity, [&assetManager](auto& mc) {
                std::string previewName = mc.MeshAsset ? mc.MeshAsset->GetName() : "no mesh";
                if (ImGui::BeginCombo("Mesh Asset", previewName.c_str())) {
                    for (const auto& [name, meshPtr] : assetManager.GetMeshes()) {
                        bool isSelected = (mc.MeshAsset == meshPtr);
                        if (ImGui::Selectable(name.c_str(), isSelected)) mc.MeshAsset = meshPtr;
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                });

            // 4. MATERIAL COMPONENT
            DrawComponent<MaterialComponent>("MaterialComponent", entity, [&assetManager](auto& matComp) {
                std::string previewName = matComp.materialAsset ? matComp.materialAsset->GetName() : "no material";
                if (ImGui::BeginCombo("Material", previewName.c_str())) {
                    for (const auto& [name, matPtr] : assetManager.GetMaterials()) {
                        bool isSelected = (matComp.materialAsset == matPtr);
                        if (ImGui::Selectable(name.c_str(), isSelected)) matComp.materialAsset = matPtr;
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                });

            // 5. CAMERA COMPONENT
            DrawComponent<CameraComponent>("CameraComponent", entity, [&scene, entity](auto& camComp) {
                auto& camera = camComp.SceneCamera;

                // --- NEU: Primary Checkbox ---
                bool isPrimary = camComp.Primary;
                if (ImGui::Checkbox("Primary", &isPrimary)) {
                    camComp.Primary = isPrimary;

                    // Wenn diese Kamera als Primary markiert wurde, alle anderen auf false setzen
                    if (isPrimary) {
                        auto view = scene.m_Registry.view<CameraComponent>();
                        for (auto otherEntityID : view) {
                            // Überspringe die aktuelle Entity
                            if (otherEntityID != (entt::entity)entity) {
                                view.get<CameraComponent>(otherEntityID).Primary = false;
                            }
                        }
                    }
                }

                // --- NEU: Fixed Aspect Ratio Checkbox ---
                ImGui::Checkbox("Fixed Aspect Ratio", &camComp.FixedAspectRatio);
                ImGui::Separator();

                // Projection Type (als int casten für ImGui Combo)
                int projType = (int)camera.GetProjectionType();
                const char* projTypeStrings[] = { "Perspective", "Orthographic" };
                if (ImGui::Combo("Projection", &projType, projTypeStrings, 2)) {
                    camera.SetProjectionType((Camera::ProjectionType)projType);
                }

                if (projType == 0) { // Perspective
                    float fov = glm::degrees(camera.GetPerspectiveVerticalFOV());
                    if (ImGui::DragFloat("FOV", &fov, 0.1f))
                        camera.SetPerspective(glm::radians(fov), camera.GetAspectRatio(), camera.GetNearClip(), camera.GetFarClip());
                }
                else { // Orthographic
                    float size = camera.GetOrthographicSize();
                    if (ImGui::DragFloat("Size", &size, 0.1f))
                        camera.SetOrthographic(size, camera.GetNearClip(), camera.GetFarClip());
                }

                // Near & Far Clip (für beide relevant)
                float nearClip = camera.GetNearClip();
                float farClip = camera.GetFarClip();
                if (ImGui::DragFloat("Near Clip", &nearClip, 0.1f) || ImGui::DragFloat("Far Clip", &farClip, 0.1f)) {
                    if (projType == 0) camera.SetPerspective(camera.GetPerspectiveVerticalFOV(), camera.GetAspectRatio(), nearClip, farClip);
                    else camera.SetOrthographic(camera.GetOrthographicSize(), nearClip, farClip);
                }
                });

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // COMPONENT HINZUFÜGEN
            if (ImGui::Button("Add Component")) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup")) {
                if (!entity.HasComponent<TransformComponent>() && ImGui::MenuItem("TransformComponent")) {
                    entity.AddComponent<TransformComponent>();
                    ImGui::CloseCurrentPopup();
                }
                if (!entity.HasComponent<MeshComponent>() && ImGui::MenuItem("MeshComponent")) {
                    entity.AddComponent<MeshComponent>();
                    ImGui::CloseCurrentPopup();
                }
                if (!entity.HasComponent<MaterialComponent>() && ImGui::MenuItem("MaterialComponent")) {
                    entity.AddComponent<MaterialComponent>();
                    ImGui::CloseCurrentPopup();
                }
                if (!entity.HasComponent<CameraComponent>() && ImGui::MenuItem("CameraComponent")) {
                    entity.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }

        if (entityDeleted) {
            renderer.getDevice().getDevice().waitIdle();
            entityToDestroy = entity;
        }

        ImGui::PopID();
    }

    // Entity erst nach der Schleife löschen, um Iterator-Invalidierung zu vermeiden
    if (entityToDestroy) {
        scene.DestroyEntity(entityToDestroy);
    }

    ImGui::End();



    // --- ASSET MANAGER WINDOW ---
    ImGui::Begin("Asset Manager");

    if (ImGui::BeginTabBar("AssetTabs")) {

        // ==========================================
        // TAB: TEXTURES
        // ==========================================
        if (ImGui::BeginTabItem("Textures")) {
            ImGui::Text("Loaded Textures:");
            ImGui::Separator();

            std::string textureToDelete = "";

            for (const auto& [name, texture] : assetManager.GetTextures()) {
                ImGui::PushID(name.c_str());

                ImGui::Text("%s", name.c_str());

                ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f);
                if (ImGui::Button("X")) {
                    textureToDelete = name;
                }

                ImGui::PopID();
            }

            if (!textureToDelete.empty()) {
                renderer.getDevice().getDevice().waitIdle();
                assetManager.RemoveTexture(textureToDelete);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Load New Texture");

            static std::string newTextureName = "";
            static std::string newTexturePath = "";

            ImGui::InputText("Name", &newTextureName);
            ImGui::InputText("Filepath", &newTexturePath);

            // --- NEU: Der File Browser Button ---
            ImGui::SameLine();
            if (ImGui::Button("Browse...")) {
                // Wir filtern direkt nach gängigen Bildformaten
                std::string filepath = FileDialogs::OpenFile("Image Files (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0All Files (*.*)\0*.*\0");

                if (!filepath.empty()) {
                    newTexturePath = filepath; // Schreibt den absoluten Pfad ins Textfeld

                    size_t lastSlash = filepath.find_last_of("/\\");
                    size_t lastDot = filepath.find_last_of(".");

                    // Wenn wir beides gefunden haben, schneiden wir genau das Wort dazwischen aus
                    if (lastSlash != std::string::npos && lastDot != std::string::npos && lastDot > lastSlash) {
                        newTextureName = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
                    }
                    else {
                        newTextureName = "NewTexture"; // Fallback
                    }
                }
            }

            if (ImGui::Button("Load Texture")) {
                // Nur laden, wenn der Nutzer einen Pfad und einen Namen angegeben hat
                if (!newTexturePath.empty() && !newTextureName.empty()) {
                    try {
                        // Der AssetManager übernimmt ab hier! Er checkt auf Duplikate und lädt die Textur.
                        assetManager.LoadTexture(renderer.getDevice(), newTextureName, newTexturePath);

                        // Felder nach erfolgreichem Laden wieder leeren
                        newTexturePath = "";
                        newTextureName = "";
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Failed to load texture: " << e.what() << std::endl;
                    }
                }
            }
            ImGui::EndTabItem();
        }

        // ==========================================
    // TAB: MATERIALS
    // ==========================================
        if (ImGui::BeginTabItem("Materials")) {
            ImGui::Text("Loaded Materials:");
            ImGui::Separator();

            std::string materialToDelete = "";

            for (const auto& [name, material] : assetManager.GetMaterials()) {
                ImGui::PushID(name.c_str());

                // TreeNode, damit man das Material aufklappen kann
                bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);

                // "X"-Button rechtsbündig
                ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f);
                if (ImGui::Button("X")) {
                    materialToDelete = name;
                }

                // Wenn das Material ausgeklappt ist: Textur bearbeiten!
                if (open) {
                    std::string albedoName = assetManager.GetTextureName(material->getTexture());

                    if (ImGui::BeginCombo("Albedo Texture", albedoName.c_str())) {
                        for (const auto& [texName, texture] : assetManager.GetTextures()) {
                            bool isSelected = (material->getTexture() == texture);

                            if (ImGui::Selectable(texName.c_str(), isSelected)) {
                                // HIER PASSIRt DAS UPDATE:
                                renderer.UpdateMaterialTexture(material, texture);
                            }

                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (!materialToDelete.empty()) {
                renderer.getDevice().getDevice().waitIdle();
                assetManager.RemoveMaterial(materialToDelete);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Create New Material");

            static std::string newMatName = "NewMaterial";
            ImGui::InputText("Material Name", &newMatName);

            // Albedo Textur auswählen
            static std::shared_ptr<Texture> selectedAlbedo = nullptr;
            std::string previewName = selectedAlbedo ? assetManager.GetTextureName(selectedAlbedo) : "Select Texture...";

            if (ImGui::BeginCombo("Albedo Texture", previewName.c_str())) {
                for (const auto& [name, texture] : assetManager.GetTextures()) {
                    bool isSelected = (selectedAlbedo == texture);
                    if (ImGui::Selectable(name.c_str(), isSelected)) {
                        selectedAlbedo = texture;
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Create Material")) {
                if (!newMatName.empty() && selectedAlbedo != nullptr) {
                    // HIER IST DIE VULKAN LOGIK EINGEBAUT:
                    // Wir nutzen die existierende Methode des Renderers, um das Material inkl. DescriptorSet zu erstellen.
                    auto newMaterial = renderer.createMaterial(newMatName, selectedAlbedo);

                    // Wir fügen es dem AssetManager hinzu, damit das restliche UI (z.B. MaterialComponent) es findet.
                    assetManager.AddMaterial(newMatName, newMaterial);

                    // Zurücksetzen für das nächste Material
                    selectedAlbedo = nullptr;
                    newMatName = "NewMaterial";
                }
            }
            ImGui::EndTabItem();
        }

        // ==========================================
        // TAB: MESHES
        // ==========================================
        if (ImGui::BeginTabItem("Meshes")) {
            ImGui::Text("Loaded Meshes:");
            ImGui::Separator();

            // Liste aller geladenen Meshes
            for (const auto& [name, mesh] : assetManager.GetMeshes()) {
                ImGui::BulletText("%s (Indices: %u)", name.c_str(), mesh->getIndexCount());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextDisabled("Mesh loading coming soon...");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End(); // Asset Manager Window beenden

    // ==========================================
    // IMGUIZMO LOGIK
    // ==========================================

    if (ImGui::IsKeyPressed(ImGuiKey_Q)) m_GizmoType = -1;
    if (ImGui::IsKeyPressed(ImGuiKey_T)) m_GizmoType = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GizmoType = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_S)) m_GizmoType = ImGuizmo::SCALE;

    // Zuerst prüfen wir, ob überhaupt eine Entity ausgewählt ist und ein Transform hat
    if (m_SelectedEntity && m_SelectedEntity.HasComponent<TransformComponent>()) {

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
            auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
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