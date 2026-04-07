#include "ImGuiLayer.hpp"


ImGuiLayer::ImGuiLayer(const VulkanDevice& device, const VulkanSwapChain& swapChain)
	: Layer("ImGuiLayer"), m_Device(device), m_SwapChain(swapChain)
{
}

void ImGuiLayer::OnAttach()
{
    // 1. ImGui Kontext erstellen
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Tastatur erlauben
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Falls du Docking nutzen willst
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    // 2. GLFW Backend initialisieren
    // Nutzt jetzt deinen neuen Getter!
    ImGui_ImplGlfw_InitForVulkan(m_Device.getWindow().getNativeWindow(), true);

    // 3. Vulkan Backend initialisieren
    ImGui_ImplVulkan_InitInfo init_info = {};

    // Nutzt jetzt deinen neuen Getter für den Context!
    init_info.Instance = *m_Device.getContext().getInstance();
    init_info.PhysicalDevice = *m_Device.getPhysicalDevice();
    init_info.Device = *m_Device.getDevice();
    init_info.QueueFamily = m_Device.getQueueIndex();
    init_info.Queue = *m_Device.getQueue();

    // ImGui erstellt seinen eigenen Pool
    //init_info.DescriptorPool = static_cast<VkDescriptorPool>(*m_ImGuiPool);
    init_info.DescriptorPoolSize = 1000;

    // Für Dynamic Rendering braucht ImGui die genauen Formate
    init_info.UseDynamicRendering = true;

    m_ColorFormat = static_cast<VkFormat>(m_SwapChain.getSurfaceFormat().format);
    init_info.PipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_ColorFormat;

    // Dynamische Frame-Limits anstatt harter Zahlen
    uint32_t imageCount = static_cast<uint32_t>(m_SwapChain.getImages().size());
    init_info.MinImageCount = imageCount;
    init_info.ImageCount = imageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // static_cast<VkSampleCountFlagBits>(device.getMsaaSamples());

    ImGui_ImplVulkan_Init(&init_info);
}

void ImGuiLayer::OnDetach()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::OnEvent(Event& e)
{
    if (m_BlockEvents)
    {
        ImGuiIO& io = ImGui::GetIO();
        e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
        e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
    }
}

void ImGuiLayer::beginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void ImGuiLayer::endFrame()
{
    ImGui::Render();
}