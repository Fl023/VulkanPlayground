#pragma once

#include "VulkanDevice.hpp"

class VulkanImage {
public:
    VulkanImage(const VulkanDevice& device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::ImageAspectFlags aspectFlags);

    // Standard-Destruktor (RAII regelt alles)
    ~VulkanImage() = default;

    // Kopieren verbieten (Vulkan Ressourcen sind unique)
    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;

    const vk::raii::Image& getImage() const { return image; }
	const vk::raii::DeviceMemory& getImageMemory() const { return imageMemory; }
    const vk::raii::ImageView& getImageView() const { return imageView; }

    vk::Format getFormat() const { return format; }
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }

private:
    const VulkanDevice& device;
    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory imageMemory = nullptr;
    vk::raii::ImageView imageView = nullptr;

    vk::Format format;
    uint32_t width;
    uint32_t height;
    
};