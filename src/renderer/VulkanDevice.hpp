#pragma once

#include "VulkanContext.hpp"
#include "VulkanWindow.hpp"

class VulkanDevice {
public:
    VulkanDevice(const VulkanContext& context, const VulkanWindow& window);
    ~VulkanDevice() = default;

    vk::raii::CommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer) const;

    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) const;
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) const;

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    void transitionImageLayout(
        vk::raii::CommandBuffer& commandBuffer,
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        uint32_t mipLevels,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::ImageAspectFlags imageAspectFlags) const;

    const VulkanContext& getContext() const;
    const vk::raii::Device& getDevice() const;
    const vk::raii::PhysicalDevice& getPhysicalDevice() const;
	const VulkanWindow& getWindow() const;
    const vk::raii::SurfaceKHR& getSurface() const;
    const vk::raii::Queue& getQueue() const;
    const uint32_t& getQueueIndex() const;
    const vk::raii::CommandPool& getCommandPool() const { return commandPool; }
    vk::Sampler getDefaultSampler() const { return *defaultSampler; }
    vk::SampleCountFlagBits getMsaaSamples() const { return msaaSamples; };


private:
    void pickPhysicalDevice();
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& device) const;
    void createLogicalDevice();
    void createDefaultSampler();
    vk::SampleCountFlagBits getMaxUsableSampleCount();

private:
    const VulkanContext& context;
    const VulkanWindow& window;

    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue queue = nullptr;
    uint32_t queueIndex = ~0;
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::Sampler defaultSampler = nullptr;
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;

    const VpProfileProperties profile = { VP_KHR_ROADMAP_2022_NAME, VP_KHR_ROADMAP_2022_SPEC_VERSION };
    std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};