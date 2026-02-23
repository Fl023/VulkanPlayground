#pragma once

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"

class Texture {
public:
    Texture(const VulkanDevice& device, const std::string& filePath);
    ~Texture();

    vk::DescriptorImageInfo GetDescriptorInfo() const {
        return vk::DescriptorImageInfo(m_Sampler, *m_ImageView, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

private:
    std::optional<VulkanImage> m_Image;
    vk::raii::ImageView m_ImageView = nullptr;
    vk::Sampler m_Sampler;
};