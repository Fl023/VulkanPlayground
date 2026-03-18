#pragma once

#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/AssetManager.hpp"
#include "renderer/VulkanRenderer.hpp"

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;
    SceneHierarchyPanel(Scene* context);

    // Setzt die aktive Szene (Raw-Pointer, da wir nur Observer sind)
    void SetContext(Scene* context);

    // Zeichnet die ImGui Fenster (Hierarchy & Properties)
    void OnImGuiRender(AssetManager& assetManager, VulkanRenderer& renderer);

    // Getter & Setter für die Auswahl (wichtig für den ImGuizmo in der EditorUI!)
    Entity GetSelectedEntity() const { return m_SelectionContext; }
    void SetSelectedEntity(Entity entity) { m_SelectionContext = entity; }

private:
    // Zeichnet einen einzelnen Knoten im Hierarchy-Baum
    void DrawEntityNode(Entity entity);
    
    // Zeichnet den Inspector/Properties-Tab für die ausgewählte Entity
    void DrawComponents(Entity entity, AssetManager& assetManager, VulkanRenderer& renderer);

    // Helper-Template für das "Add Component" Menü (vermeidet Code-Duplizierung)
    template<typename T>
    void DisplayAddComponentEntry(const std::string& entryName);

private:
    Scene* m_Context = nullptr;
    Entity m_SelectionContext{}; // selected Entity
};