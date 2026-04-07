#pragma once

#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/AssetManager.hpp"
#include "renderer/VulkanRenderer.hpp"
#include "SceneHierarchyPanel.hpp"
#include "ContentBrowserPanel.hpp"

enum class SceneState
{
    Edit = 0,
    Play = 1
};

class EditorUI
{
public:
    EditorUI() = default;
    ~EditorUI() = default;

    void Draw(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer, std::function<void(const std::string&)> onSceneOpen);
    void DrawToolbar(SceneState& currentState, std::function<void()> onPlay, std::function<void()> onStop);

    glm::vec2 GetViewportSize() const { return { m_ViewportSize.x, m_ViewportSize.y }; }

private:
    void BeginDockspace();
    void EndDockspace();
    void DrawMenuBar(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer, std::function<void(const std::string&)> onSceneOpen);
    void DrawViewport(Scene& scene, VulkanRenderer& renderer);
    void DrawGizmos(Scene& scene, ImVec2 viewportOffset, ImVec2 viewportSize);

private:
    SceneHierarchyPanel m_SceneHierarchyPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    Scene* m_CurrentScene = nullptr;
    int m_GizmoType = ImGuizmo::TRANSLATE;
    ImVec2 m_ViewportSize = { 0.0f, 0.0f };
};