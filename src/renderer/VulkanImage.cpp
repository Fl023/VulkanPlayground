#include "VulkanImage.hpp"
#include "VulkanBuffer.hpp"

VulkanImage::VulkanImage(const VulkanDevice& device, uint32_t width, uint32_t height, 
    uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, 
    vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, 
    vk::ImageAspectFlags aspectFlags, bool isCubeMap)
	: device(device), format(format), width(width), height(height)
{
    uint32_t layerCount = isCubeMap ? 6 : 1;
    vk::ImageCreateFlags createFlags = isCubeMap ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags{};

    vk::ImageCreateInfo imageInfo{ 
        .flags = createFlags,
        .imageType = vk::ImageType::e2D, 
        .format = format,
        .extent = {width, height, 1}, 
        .mipLevels = mipLevels, 
        .arrayLayers = layerCount,
        .samples = numSamples, 
        .tiling = tiling,
        .usage = usage, 
        .sharingMode = vk::SharingMode::eExclusive 
    };

    image = vk::raii::Image(device.getDevice(), imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size,
                                        .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties) };
    imageMemory = vk::raii::DeviceMemory(device.getDevice(), allocInfo);
    image.bindMemory(imageMemory, 0);

    vk::ImageViewCreateInfo viewInfo{
        .image = *image,
        .viewType = isCubeMap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = layerCount
        }
    };

    imageView = vk::raii::ImageView(device.getDevice(), viewInfo);
}

