#pragma once

#include "VulkanDevice.hpp"
#include "VulkanImage.hpp"

class RenderTarget
{
public:
    RenderTarget(const VulkanDevice& device, uint32_t width, uint32_t height, vk::Format colorFormat);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    void Resize(uint32_t width, uint32_t height);

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

    // Provide the raw images for Dynamic Rendering
    VulkanImage& GetColorImage() const { return *m_ColorImage; }
    VulkanImage& GetDepthImage() const { return *m_DepthImage; }
    VulkanImage& GetResolveImage() const { return *m_ResolveImage; }
    
    vk::DescriptorSet GetImGuiTextureID() const { return m_ImGuiDescriptorSet; }

private:
    void Invalidate();
    void CreateResources();

private:
    const VulkanDevice& m_Device;
    uint32_t m_Width;
    uint32_t m_Height;
    vk::Format m_ColorFormat;

    std::unique_ptr<VulkanImage> m_ColorImage;
    std::unique_ptr<VulkanImage> m_DepthImage;
    std::unique_ptr<VulkanImage> m_ResolveImage;
    
    vk::raii::Sampler m_Sampler = nullptr;
    vk::DescriptorSet m_ImGuiDescriptorSet = nullptr; 
};