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
        auto view = m_Context->m_Registry.view<TagComponent>();
        for (auto entityID : view)
        {
            Entity entity{ entityID , m_Context };
            DrawEntityNode(entity, renderer);
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
    }
    ImGui::End();

    ImGui::Begin("Properties");
    if (m_SelectionContext)
    {
        DrawComponents(m_SelectionContext, assetManager, renderer);
    }
    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity, VulkanRenderer& renderer)
{
    auto& tag = entity.GetComponent<TagComponent>().Tag;
    
    ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= ImGuiTreeNodeFlags_Leaf;
    
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
    if (ImGui::IsItemClicked())
    {
        m_SelectionContext = entity;
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
        // Placeholder für Child-Entities
        ImGui::TreePop();
    }

    if (entityDeleted)
    {
        m_Context->DestroyEntity(entity);
        if (m_SelectionContext == entity)
            m_SelectionContext = {};
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
        ImGui::EndPopup();
    }
    ImGui::PopItemWidth();

    DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
    {
        DrawVec3Control("Translation", component.Translation);
        glm::vec3 rotation = glm::degrees(component.Rotation);
        DrawVec3Control("Rotation", rotation);
        component.Rotation = glm::radians(rotation);
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

}