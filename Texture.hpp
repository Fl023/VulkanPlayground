#pragma once

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"

class Texture {
public:
    Texture(const VulkanDevice& device, const std::string& filePath);
    ~Texture();

    vk::DescriptorImageInfo GetDescriptorInfo() const {
        return vk::DescriptorImageInfo(m_Sampler, *m_Image->getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
    }

private:
    void generateMipmaps();

private:
    const VulkanDevice& m_device;
    std::optional<VulkanImage> m_Image;
    uint32_t m_mipLevels;
    int m_texWidth, m_texHeight;
    vk::Sampler m_Sampler;
};