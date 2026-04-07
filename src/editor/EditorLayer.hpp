#pragma once
#include "Core/Layer.hpp"
#include "scene/Scene.hpp"
#include "scene/AssetManager.hpp"
#include "editor/EditorUI.hpp"
#include "renderer/SceneRenderer.hpp"
#include "editor/EditorCamera.hpp"

class EditorLayer : public Layer
{
public:
	EditorLayer(AssetManager* assetManager, VulkanRenderer* renderer);

	~EditorLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnUpdate(Timestep ts) override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(Event& event) override;

private:
	void OnScenePlay();
	void OnSceneStop();
	void OpenScene(const std::string& filepath);

private:
	EditorUI m_EditorUI;
	AssetManager* m_AssetManager = nullptr;
	VulkanRenderer* m_Renderer = nullptr;
	std::unique_ptr<SceneRenderer> m_SceneRenderer;

	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	EditorCamera m_EditorCamera;
	SceneState m_SceneState = SceneState::Edit;

	std::shared_ptr<Scene> m_EditorScene;
	std::shared_ptr<Scene> m_ActiveScene;
};