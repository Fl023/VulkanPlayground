#pragma once

#include "renderer/VulkanRenderer.hpp"
#include "events/Event.hpp"
#include "events/ApplicationEvent.hpp"
#include "core/LayerStack.hpp"
#include "renderer/SceneRenderer.hpp"
#include "renderer/ImGuiLayer.hpp"
#include "scene/AssetManager.hpp"

class Application
{
public:
    Application(const std::string& name = "Vulkan Engine", uint32_t width = 1920, uint32_t height = 1080);
    virtual ~Application();

    void Run();
    void Close();

    void QueueEvent(std::unique_ptr<Event> e);

    void OnEvent(Event& e);

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    void SetSceneRenderer(std::unique_ptr<SceneRenderer> customRenderer);

    void LoadScene(std::shared_ptr<Scene> newScene) { m_ActiveScene = newScene; }

    VulkanWindow& GetWindow() { return *m_Window; }
    VulkanRenderer& GetRenderer() { return *m_Renderer; }
    Scene& GetActiveScene() { return *m_ActiveScene; }
    AssetManager& GetAssetManager() { return m_AssetManager; }

    static Application& Get() { return *s_Instance; }

private:
    void ProcessEvents();

    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

protected:
    std::unique_ptr<VulkanWindow> m_Window;
    std::unique_ptr<VulkanRenderer> m_Renderer;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;
    AssetManager m_AssetManager;
    std::shared_ptr<Scene> m_ActiveScene;

	ImGuiLayer* m_ImGuiLayer;
    LayerStack m_LayerStack;
    std::vector<std::unique_ptr<Event>> m_EventQueue;

    bool m_Running = true;
    bool m_Minimized = false;
    float m_LastFrameTime = 0.0f;

    static Application* s_Instance;
};