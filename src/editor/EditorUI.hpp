#pragma once

#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/AssetManager.hpp"
#include "renderer/VulkanRenderer.hpp"
#include "SceneHierarchyPanel.hpp"
#include "ContentBrowserPanel.hpp"

class EditorUI
{
public:
    EditorUI() = default;
    ~EditorUI() = default;

    // Diese Methode wird jeden Frame aufgerufen
    void Draw(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer);

private:
    SceneHierarchyPanel m_SceneHierarchyPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    Scene* m_CurrentScene = nullptr;
    int m_GizmoType = ImGuizmo::TRANSLATE;
};