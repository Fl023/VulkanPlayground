#include "SceneHierarchyPanel.hpp"
#include "scene/Components.hpp"

SceneHierarchyPanel::SceneHierarchyPanel(Scene* context)
{
    SetContext(context);
}

void SceneHierarchyPanel::SetContext(Scene* context)
{
    m_Context = context;
    m_SelectionContext = {};
}

// ==============================================================================
// UI HELPER FUNKTIONEN (Inspiriert von Hazel, angepasst für uns)
// ==============================================================================

// Hazels geniale Vektor-Kontrolle (X=Rot, Y=Grün, Z=Blau)
static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
{
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0]; // Später hier deinen Bold-Font einsetzen

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    // --- X Achse (Rot) ---
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize)) values.x = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // --- Y Achse (Grün) ---
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize)) values.y = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // --- Z Achse (Blau) ---
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize)) values.z = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
}

template<typename T, typename UIFunction>
static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, VulkanRenderer* renderer = nullptr)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
    
    if (entity.HasComponent<T>())
    {
        auto& component = entity.GetComponent<T>();
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImGui::Separator();
        
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
        ImGui::PopStyleVar();
        
        // Settings Menü rechtsbündig
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
        {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings"))
        {
            if (ImGui::MenuItem("Remove component"))
                removeComponent = true;
            ImGui::EndPopup();
        }

        if (open)
        {
            uiFunction(component);
            ImGui::TreePop();
        }

        if (removeComponent) {
            entity.RemoveComponent<T>();
        }   
    }
}

// Hazels Template-Helper für das AddComponent Menü
template<typename T>
void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) 
{
    if (!m_SelectionContext.HasComponent<T>())
    {
        if (ImGui::MenuItem(entryName.c_str()))
        {
            m_SelectionContext.AddComponent<T>();
            ImGui::CloseCurrentPopup();
        }
    }
}

// ==============================================================================
// CORE PANEL METHODEN
// ==============================================================================

void SceneHierarchyPanel::OnImGuiRender(AssetManager& assetManager, VulkanRenderer& renderer)
{
    ImGui::Begin("Scene Hierarchy");

    if (m_Context)
    {
        // 1. Wrap the entire hierarchy in a Child Window
        // This takes up all available space and gives us a giant background to drop onto
        ImGui::BeginChild("HierarchyTree");

        auto view = m_Context->m_Registry.view<RelationshipComponent>();
        for (auto entityID : view)
        {
            Entity entity{ entityID , m_Context };
            const auto& rel = entity.GetComponent<RelationshipComponent>();
            if (rel.Parent == entt::null)
                DrawEntityNode(entity);
        }

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            m_SelectionContext = {};

        // Rechtsklick ins Leere = Neue Entity (Wie bei Hazel)
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
                m_Context->CreateEntity("Empty Entity");
            ImGui::EndPopup();
        }

        ImGui::EndChild(); // <-- End of the Hierarchy list

        // 3. Attach the Drop Target to the entire Child Window we just finished drawing!
        // This catches any drops that happen "between" the nodes or in the background.
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
            {
                entt::entity droppedEntityHandle = *(const entt::entity*)payload->Data;
                Entity droppedEntity{ droppedEntityHandle, m_Context };

                // Disconnect from parent, returning it to the root level
                droppedEntity.Unlink();
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::End();

    ImGui::Begin("Properties");
    if (m_Context && m_SelectionContext && m_Context->m_Registry.valid((entt::entity)m_SelectionContext))
    {
        DrawComponents(m_SelectionContext, assetManager, renderer);
    }
    else if (m_SelectionContext)
    {
        // If it was selected but is no longer valid, clear the selection
        m_SelectionContext = {};
    }
    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity)
{
    auto& tag = entity.GetComponent<TagComponent>().Tag;
    auto& rel = entity.GetComponent<RelationshipComponent>();
    
    ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0)
        | ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_SpanAvailWidth;

    // If the entity has no children, render it as a leaf (removes the expand arrow)
    if (rel.FirstChild == entt::null) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
    if (ImGui::IsItemClicked())
    {
        m_SelectionContext = entity;
    }

    // --- 1. DRAG SOURCE ---
    if (ImGui::BeginDragDropSource()) {
        entt::entity entityHandle = entity;
        // Attach the entity handle as the payload
        ImGui::SetDragDropPayload("SCENE_HIERARCHY_ENTITY", &entityHandle, sizeof(entt::entity));
        ImGui::Text("Move %s", tag.c_str()); // Tooltip next to cursor
        ImGui::EndDragDropSource();
    }

    // --- 2. DRAG TARGET ---
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY")) {
            entt::entity droppedEntityHandle = *(const entt::entity*)payload->Data;
            Entity droppedEntity{ droppedEntityHandle, m_Context };

            // Set the dropped entity's parent to the entity we are hovering over!
            droppedEntity.SetParent(entity);
        }
        ImGui::EndDragDropTarget();
    }

    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity"))
            entityDeleted = true;
        ImGui::EndPopup();
    }

    if (opened)
    {
        entt::entity currentChild = rel.FirstChild;

        // Traverse the linked list of siblings
        while (currentChild != entt::null) {
            Entity childEntity(currentChild, m_Context);
            entt::entity nextChild = childEntity.GetComponent<RelationshipComponent>().NextSibling;
            DrawEntityNode(childEntity); // Recursion!

            // Move to the next child
            currentChild = nextChild;
        }
        ImGui::TreePop();
    }

    if (entityDeleted)
    {
        if (m_SelectionContext == entity) m_SelectionContext = {};
        m_Context->DestroyEntity(entity);
    }
}

void SceneHierarchyPanel::DrawComponents(Entity entity, AssetManager& assetManager, VulkanRenderer& renderer)
{
    if (entity.HasComponent<TagComponent>())
    {
        auto& tag = entity.GetComponent<TagComponent>().Tag;
        ImGui::InputText("##Tag", &tag);
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(-1);

    if (ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");

    if (ImGui::BeginPopup("AddComponent"))
    {
        DisplayAddComponentEntry<TransformComponent>("Transform");
        DisplayAddComponentEntry<MeshComponent>("Mesh");
        DisplayAddComponentEntry<MaterialComponent>("Material");
        DisplayAddComponentEntry<CameraComponent>("Camera");
        DisplayAddComponentEntry<SkyboxComponent>("Skybox");
        DisplayAddComponentEntry<RelationshipComponent>("Relationship");
        ImGui::EndPopup();
    }
    ImGui::PopItemWidth();

    DrawComponent<IDComponent>("ID", entity, [](auto& component)
    {
        ImGui::Text("UUID: %016llx%016llx", component.ID.GetHigh(), component.ID.GetLow());
	});


    DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
    {
        DrawVec3Control("Translation", component.Translation);
        glm::vec3 rotation = glm::degrees(component.GetEulerAngles());
        DrawVec3Control("Rotation", rotation);
        component.SetEulerAngles(glm::radians(rotation));
        DrawVec3Control("Scale", component.Scale, 1.0f);
    });

    DrawComponent<MeshComponent>("Mesh", entity, [&assetManager](auto& mc) {
        std::string previewName = "no mesh";
        // Find the name of the currently selected handle
        for (const auto& [name, handle] : assetManager.GetMeshRegistry()) {
            if (mc.MeshHandle == handle) { previewName = name; break; }
        }

        if (ImGui::BeginCombo("Asset", previewName.c_str())) {
            for (const auto& [name, handle] : assetManager.GetMeshRegistry()) {
                bool isSelected = (mc.MeshHandle == handle);
                // Assign the integer handle directly!
                if (ImGui::Selectable(name.c_str(), isSelected)) mc.MeshHandle = handle;
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }, &renderer);

    DrawComponent<MaterialComponent>("Material", entity, [&assetManager, &renderer](auto& matComp) {
        std::string previewName = "no material";
        for (const auto& [name, handle] : assetManager.GetMaterialRegistry()) {
            if (matComp.MaterialHandle == handle) { previewName = name; break; }
        }

        if (ImGui::BeginCombo("Asset", previewName.c_str())) {
            for (const auto& [name, handle] : assetManager.GetMaterialRegistry()) {
                bool isSelected = (matComp.MaterialHandle == handle);
                // Assign the integer handle directly!
                if (ImGui::Selectable(name.c_str(), isSelected)) matComp.MaterialHandle = handle;
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }, &renderer);

    DrawComponent<CameraComponent>("Camera", entity, [this](auto& component)
    {
        auto& camera = component.SceneCamera;

        bool isPrimary = component.Primary;
        if (ImGui::Checkbox("Primary", &isPrimary)) {
            component.Primary = isPrimary;
            if (isPrimary) {
                auto view = m_Context->m_Registry.view<CameraComponent>();
                for (auto otherEntityID : view) {
                    if (otherEntityID != (entt::entity)m_SelectionContext) {
                        view.get<CameraComponent>(otherEntityID).Primary = false;
                    }
                }
            }
        }

        const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
        const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
        if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
        {
            for (int i = 0; i < 2; i++)
            {
                bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
                if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
                {
                    currentProjectionTypeString = projectionTypeStrings[i];
                    camera.SetProjectionType((Camera::ProjectionType)i);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (camera.GetProjectionType() == Camera::ProjectionType::Perspective)
        {
            float fov = glm::degrees(camera.GetPerspectiveVerticalFOV());
            if (ImGui::DragFloat("Vertical FOV", &fov))
                camera.SetPerspective(glm::radians(fov), camera.GetAspectRatio(), camera.GetNearClip(), camera.GetFarClip());

            float nearClip = camera.GetNearClip();
            if (ImGui::DragFloat("Near", &nearClip))
                camera.SetPerspective(camera.GetPerspectiveVerticalFOV(), camera.GetAspectRatio(), nearClip, camera.GetFarClip());

            float farClip = camera.GetFarClip();
            if (ImGui::DragFloat("Far", &farClip))
                camera.SetPerspective(camera.GetPerspectiveVerticalFOV(), camera.GetAspectRatio(), camera.GetNearClip(), farClip);
        }

            if (camera.GetProjectionType() == Camera::ProjectionType::Orthographic)
            {
                float orthoSize = camera.GetOrthographicSize();
                if (ImGui::DragFloat("Size", &orthoSize))
                    camera.SetOrthographic(orthoSize, camera.GetNearClip(), camera.GetFarClip());

                float nearClip = camera.GetNearClip();
                if (ImGui::DragFloat("Near", &nearClip))
                    camera.SetOrthographic(camera.GetOrthographicSize(), nearClip, camera.GetFarClip());

                float farClip = camera.GetFarClip();
                if (ImGui::DragFloat("Far", &farClip))
                    camera.SetOrthographic(camera.GetOrthographicSize(), camera.GetNearClip(), farClip);

                ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
            }
        });

    DrawComponent<SkyboxComponent>("Skybox", entity, [&assetManager](auto& component)
    {
        std::string previewName = "No Cubemap";

        // 1. Find the name of the currently selected cubemap
        for (const auto& [name, handle] : assetManager.GetTextureRegistry()) {
            if (component.CubemapHandle == handle) {
                previewName = name;
                break;
            }
        }

        // 2. Draw the Dropdown
        if (ImGui::BeginCombo("Cubemap Asset", previewName.c_str()))
        {
            // Option to clear the skybox
            if (ImGui::Selectable("None", component.CubemapHandle == INVALID_ASSET_HANDLE)) {
                component.CubemapHandle = INVALID_ASSET_HANDLE;
            }

            // Loop through all loaded textures
            for (const auto& [name, handle] : assetManager.GetTextureRegistry())
            {
                Texture* tex = assetManager.GetTexture(handle);

                // ONLY show the texture in the dropdown if it is actually a Cubemap!
                if (tex && tex->IsCubemap())
                {
                    bool isSelected = (component.CubemapHandle == handle);
                    if (ImGui::Selectable(name.c_str(), isSelected)) {
                        component.CubemapHandle = handle;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::EndCombo();
        }
    });

    DrawComponent<RelationshipComponent>("Relationship", entity, [this](auto& component)
    {
        // Helper lambda to safely get the name of an entity
        auto getEntityName = [this](entt::entity ent) -> std::string {
            if (ent == entt::null || !m_Context->m_Registry.valid(ent)) return "None";

            Entity e{ ent, m_Context };
            if (e.HasComponent<TagComponent>())
                return e.GetComponent<TagComponent>().Tag;

            return "Unnamed Entity";
            };

        // Fetch the names of the relationships
        std::string parentName = getEntityName(component.Parent);
        std::string firstChildName = getEntityName(component.FirstChild);
        std::string prevSiblingName = getEntityName(component.PrevSibling);
        std::string nextSiblingName = getEntityName(component.NextSibling);

        // Display them as read-only text in the UI
        ImGui::Text("Parent: %s", parentName.c_str());
        ImGui::Text("First Child: %s", firstChildName.c_str());
        ImGui::Text("Previous Sibling: %s", prevSiblingName.c_str());
        ImGui::Text("Next Sibling: %s", nextSiblingName.c_str());
    });

}