#pragma once

#include "renderer/VulkanRenderer.hpp"
#include "events/Event.hpp"
#include "events/ApplicationEvent.hpp"
#include "core/LayerStack.hpp"
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

    VulkanWindow& GetWindow() { return *m_Window; }
    VulkanRenderer& GetRenderer() { return *m_Renderer; }
    AssetManager& GetAssetManager() { return m_AssetManager; }

    static Application& Get() { return *s_Instance; }

private:
    void ProcessEvents();

    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

protected:
    std::unique_ptr<VulkanWindow> m_Window;
    std::unique_ptr<VulkanRenderer> m_Renderer;
    AssetManager m_AssetManager;

	ImGuiLayer* m_ImGuiLayer;
    LayerStack m_LayerStack;
    std::vector<std::unique_ptr<Event>> m_EventQueue;

    bool m_Running = true;
    bool m_Minimized = false;
    float m_LastFrameTime = 0.0f;

    static Application* s_Instance;
};