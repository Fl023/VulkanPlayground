#include "ImGuiLayer.hpp"


ImGuiLayer::ImGuiLayer(const VulkanDevice& device, const VulkanSwapChain& swapChain)
    : m_Device(device)
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
    ImGui_ImplGlfw_InitForVulkan(device.getWindow().getNativeWindow(), true);

    // 3. Vulkan Backend initialisieren
    ImGui_ImplVulkan_InitInfo init_info = {};
    
    // Nutzt jetzt deinen neuen Getter für den Context!
    init_info.Instance = *device.getContext().getInstance(); 
    init_info.PhysicalDevice = *device.getPhysicalDevice();
    init_info.Device = *device.getDevice();
    init_info.QueueFamily = device.getQueueIndex();
    init_info.Queue = *device.getQueue();

    // ImGui erstellt seinen eigenen Pool
    init_info.DescriptorPoolSize = 1000; 
    
    // Für Dynamic Rendering braucht ImGui die genauen Formate
    init_info.UseDynamicRendering = true;
    
    m_ColorFormat = static_cast<VkFormat>(swapChain.getSurfaceFormat().format);
    init_info.PipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_ColorFormat;
    
    // Dynamische Frame-Limits anstatt harter Zahlen
    uint32_t imageCount = static_cast<uint32_t>(swapChain.getImages().size());
    init_info.MinImageCount = imageCount; 
    init_info.ImageCount = imageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // static_cast<VkSampleCountFlagBits>(device.getMsaaSamples());

    ImGui_ImplVulkan_Init(&init_info);
}

ImGuiLayer::~ImGuiLayer()
{
    // Warten, bis die GPU nichts mehr macht, bevor Ressourcen freigegeben werden
    m_Device.getDevice().waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::beginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void ImGuiLayer::recordImGuiCommands(const vk::raii::CommandBuffer& commandBuffer)
{
    // Generiert die Geometrie für die UI
    ImGui::Render();
    
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data) {
        // vk::raii::CommandBuffer zu VkCommandBuffer dereferenzieren und aufzeichnen
        ImGui_ImplVulkan_RenderDrawData(draw_data, *commandBuffer);
    }
}