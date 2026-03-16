#pragma once

#include "renderer/VulkanDevice.hpp"
#include "renderer/VulkanBuffer.hpp"
#include "renderer/VulkanImage.hpp"

class Texture {
public:
    Texture(const VulkanDevice& device, const std::string& filePath);
    Texture(const VulkanDevice& device, uint32_t width, uint32_t height, const void* pixels);
    Texture(const VulkanDevice& device, const std::array<std::string, 6>& facePaths);
    ~Texture();

    vk::DescriptorImageInfo GetDescriptorInfo() const {
        return vk::DescriptorImageInfo(m_Sampler, *m_Image->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    const std::string& GetFilePath() const { return m_FilePath; }

    void SetBindlessIndex(uint32_t index) { m_BindlessIndex = index; }
    uint32_t GetBindlessIndex() const { return m_BindlessIndex; }

    bool IsCubemap() const { return m_IsCubemap; }

private:
    void generateMipmaps();

    void createVulkanImage(const void* pixels, vk::DeviceSize imageSize);

private:
    std::string m_FilePath;
    const VulkanDevice& m_device;
    std::optional<VulkanImage> m_Image;
    uint32_t m_mipLevels;
    int m_texWidth, m_texHeight;
    vk::Sampler m_Sampler;
    uint32_t m_BindlessIndex = 0;
    bool m_IsCubemap = false;
};