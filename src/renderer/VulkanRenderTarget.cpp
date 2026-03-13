#include "VulkanRenderTarget.hpp"

RenderTarget::RenderTarget(const VulkanDevice& device, uint32_t width, uint32_t height, vk::Format colorFormat)
    : m_Device(device), m_Width(width), m_Height(height), m_ColorFormat(colorFormat)
{
    CreateResources();
}

RenderTarget::~RenderTarget()
{
    Invalidate();
}

void RenderTarget::Resize(uint32_t width, uint32_t height)
{
    if (width == m_Width && height == m_Height) return;
    if (width == 0 || height == 0) return; // Prevent Vulkan crash on minimizing

    m_Device.getDevice().waitIdle();

    m_Width = width;
    m_Height = height;

    Invalidate();
    CreateResources();
}

void RenderTarget::Invalidate()
{
    if (m_ImGuiDescriptorSet) {
        ImGui_ImplVulkan_RemoveTexture(m_ImGuiDescriptorSet);
        m_ImGuiDescriptorSet = nullptr;
    }

    m_Sampler = nullptr;
    m_ColorImage.reset();
    m_DepthImage.reset();
    m_ResolveImage.reset(); // Destroy the resolve image too!
}

void RenderTarget::CreateResources()
{
    // Fetch the max samples you calculated during device creation!
    vk::SampleCountFlagBits msaaSamples = m_Device.getMsaaSamples();

    // ==========================================
    // 1. CREATE MSAA COLOR ATTACHMENT (The Canvas)
    // ==========================================
    // This image ONLY needs to be a ColorAttachment. ImGui will never read this directly!
    m_ColorImage = std::make_unique<VulkanImage>(
        m_Device, m_Width, m_Height, 1, 
        msaaSamples, // <--- Using your MSAA count!
        m_ColorFormat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eColorAttachment, // Only drawn to, not sampled
        vk::MemoryPropertyFlagBits::eDeviceLocal, 
        vk::ImageAspectFlagBits::eColor
    );

    // ==========================================
    // 2. CREATE MSAA DEPTH ATTACHMENT (The Z-Buffer)
    // ==========================================
    // Depth buffers must always match the sample count of the color buffer!
    m_DepthImage = std::make_unique<VulkanImage>(
        m_Device, m_Width, m_Height, 1, 
        msaaSamples, // <--- Using your MSAA count!
        vk::Format::eD32Sfloat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eDepthStencilAttachment, 
        vk::MemoryPropertyFlagBits::eDeviceLocal, 
        vk::ImageAspectFlagBits::eDepth
    );

    // ==========================================
    // 3. CREATE RESOLVE ATTACHMENT (The ImGui Texture)
    // ==========================================
    // This is the flattened, 1-sample image. 
    // It must be a ColorAttachment (to receive the resolve) AND Sampled (for ImGui).
    m_ResolveImage = std::make_unique<VulkanImage>(
        m_Device, m_Width, m_Height, 1, 
        vk::SampleCountFlagBits::e1, // <--- STRICTLY 1 SAMPLE!
        m_ColorFormat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, 
        vk::MemoryPropertyFlagBits::eDeviceLocal, 
        vk::ImageAspectFlagBits::eColor
    );

    // ==========================================
    // 4. CREATE SAMPLER FOR IMGUI
    // ==========================================
    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
        .maxAnisotropy = 1.0f,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
    };
    m_Sampler = vk::raii::Sampler(m_Device.getDevice(), samplerInfo);

    // ==========================================
    // 5. REGISTER WITH IMGUI!
    // ==========================================
    // Notice we are passing the RESOLVE image view here, NOT the MSAA color view!
    m_ImGuiDescriptorSet = ImGui_ImplVulkan_AddTexture(
        static_cast<VkSampler>(*m_Sampler), 
        static_cast<VkImageView>(*m_ResolveImage->getImageView()), 
        static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal) 
    );
}