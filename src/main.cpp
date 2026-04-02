#include "renderer/VulkanRenderer.hpp"
#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/Components.hpp"
#include "core/Input.hpp"
#include "scene/AssetManager.hpp"
#include "editor/EditorUI.hpp"
#include "scene/Model.hpp"
#include "scene/SceneSerializer.hpp"


#include "core/Application.hpp"

class GameLayer : public Layer
{
	public:
	GameLayer() : Layer("Game Layer") {}
	~GameLayer() = default;
	virtual void OnAttach() override {}
	virtual void OnDetach() override {}
	virtual void OnUpdate(Timestep ts) override {}
	virtual void OnImGuiRender() override {}
	virtual void OnEvent(Event& event) override {}
};

class EditorLayer : public Layer
{
	public:
	EditorLayer(Scene* scene, AssetManager* assetManager, VulkanRenderer* renderer) 
		: Layer("Editor Layer"), m_CurrentScene(scene), m_AssetManager(assetManager), m_Renderer(renderer) {}
	~EditorLayer() = default;
	virtual void OnAttach() override {}
	virtual void OnDetach() override {}
	virtual void OnUpdate(Timestep ts) override {}
	virtual void OnImGuiRender() override { m_EditorUI.Draw(*m_CurrentScene, *m_AssetManager, *m_Renderer); }
	virtual void OnEvent(Event& event) override {}

private:
	EditorUI m_EditorUI;
	Scene* m_CurrentScene = nullptr;
	AssetManager* m_AssetManager = nullptr;
	VulkanRenderer* m_Renderer = nullptr;
};

class Sandbox : public Application
{
	public:
	Sandbox() : Application("Vulkan Engine Sandbox") 
	{
		PushLayer(new GameLayer());
		PushOverlay(new EditorLayer(&GetActiveScene(), &GetAssetManager(), &GetRenderer()));
	}
	~Sandbox() = default;
};

int main()
{
	try
	{
		Sandbox app;
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}