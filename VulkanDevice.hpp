#pragma once

#include "VulkanContext.hpp"
#include "VulkanWindow.hpp"

class VulkanDevice {
public:
    VulkanDevice(const VulkanContext& context, const VulkanWindow& window);
    ~VulkanDevice() = default;

    const vk::raii::Device& getDevice() const;
    const vk::raii::PhysicalDevice& getPhysicalDevice() const;
    const vk::raii::SurfaceKHR& getSurface() const;
    const vk::raii::Queue& getQueue() const;
    const uint32_t& getQueueIndex() const;

private:
    void pickPhysicalDevice();
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& device) const;
    void createLogicalDevice();

    const VulkanContext& context;
    const VulkanWindow& window;

    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue queue = nullptr;
    
    uint32_t queueIndex = ~0;

    const VpProfileProperties profile = { VP_KHR_ROADMAP_2022_NAME, VP_KHR_ROADMAP_2022_SPEC_VERSION };
    std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};