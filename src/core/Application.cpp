#include "Application.hpp"
#include "Input.hpp"
#include "scene/RenderView.hpp"
#include "renderer/RenderGraph.hpp"
#include "scene/Scene.hpp"
#include "scripts/ScriptEngine.hpp"


#include <chrono>

// Initialize the static instance pointer
Application* Application::s_Instance = nullptr;

Application::Application(const std::string& name, uint32_t width, uint32_t height)
{
    // Ensure only one Application exists
    if (!s_Instance)
    {
        s_Instance = this;
    }

    // 1. Initialize the Window
    m_Window = std::make_unique<VulkanWindow>(width, height, name);

    // Bind the Window's callback to our buffered queue
    // Now, when GLFW registers an event, it creates a unique_ptr and pushes it here
    m_Window->SetEventCallback([this](std::unique_ptr<Event> e) {
        QueueEvent(std::move(e));
        });

    Input::Init(m_Window->getNativeWindow());

    // 2. Initialize the Vulkan Renderer
    m_Renderer = std::make_unique<VulkanRenderer>(*m_Window);

    m_ImGuiLayer = new ImGuiLayer(m_Renderer->getDevice(), m_Renderer->getSwapChain());
    PushOverlay(m_ImGuiLayer);

    m_Renderer->InitViewport();

	// Initialize the Script Engine (Lua)
	ScriptEngine::Init();
}

Application::~Application()
{
    // Vulkan requires waiting for the GPU to be idle before destroying resources
    m_Renderer->getDevice().getDevice().waitIdle();

    m_Renderer->DestroyViewport();
}

void Application::Close()
{
    m_Running = false;
}

// -------------------------------------------------------------------
// EVENT QUEUE SYSTEM
// -------------------------------------------------------------------

void Application::QueueEvent(std::unique_ptr<Event> e)
{
    // Simply store the event for later processing. This is extremely fast and non-blocking.
    m_EventQueue.push_back(std::move(e));
}

void Application::ProcessEvents()
{
    // Drain the queue and route each event
    for (auto& eventPtr : m_EventQueue)
    {
        OnEvent(*eventPtr);
    }

    // Clear the queue for the next frame
    m_EventQueue.clear();
}

void Application::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);

    // Route core window events first
    dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) { return OnWindowClose(event); });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) { return OnWindowResize(event); });

    // Propagate the event backwards through the LayerStack (Overlays get it first)
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
    {
        if (e.Handled)
            break;

        (*it)->OnEvent(e);
    }
}

// -------------------------------------------------------------------
// LAYER SYSTEM
// -------------------------------------------------------------------

void Application::PushLayer(Layer* layer)
{
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer* layer)
{
    m_LayerStack.PushOverlay(layer);
    layer->OnAttach();
}

// -------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------

void Application::Run()
{
    auto startTime = std::chrono::high_resolution_clock::now();

    while (m_Running && !m_Window->shouldClose())
    {
        // 1. Poll operating system events
        // This will rapidly call QueueEvent() under the hood
        m_Window->pollEvents();

        // 2. Safely process all queued inputs before game logic starts
        ProcessEvents();

        // Calculate Delta Time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        startTime = currentTime;

        if (!m_Minimized)
        {
            // 3. Update Game Logic (Layers)
            for (Layer* layer : m_LayerStack)
                layer->OnUpdate(deltaTime);
        }

        // 4. Begin ImGui Frame
        m_ImGuiLayer->beginFrame();

        // 5. Update UI Logic (Layers)
        for (Layer* layer : m_LayerStack)
            layer->OnImGuiRender();

		m_ImGuiLayer->endFrame();

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

    }
}

// -------------------------------------------------------------------
// CORE EVENT HANDLERS
// -------------------------------------------------------------------

bool Application::OnWindowClose(WindowCloseEvent& e)
{
    m_Running = false;
    return true; // Mark as handled
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
    if (e.GetWidth() == 0 || e.GetHeight() == 0)
    {
        m_Minimized = true;
        return false;
    }

    m_Minimized = false;

    // Notify the Vulkan renderer to recreate the swapchain
    m_Renderer->framebufferResized = true;

    return false;
}
