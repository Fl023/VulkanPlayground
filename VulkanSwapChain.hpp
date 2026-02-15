#pragma once

#include "VulkanDevice.hpp"

class VulkanSwapChain
{
public:
    VulkanSwapChain(const VulkanDevice& device, const VulkanWindow& window);
    ~VulkanSwapChain() = default;

    void recreate();

    const vk::raii::SwapchainKHR& getSwapChain() const;
    const std::vector<vk::Image>& getImages() const;
    const vk::SurfaceFormatKHR& getSurfaceFormat() const;
    const vk::Extent2D& getExtent() const;
    const std::vector<vk::raii::ImageView>& getImageViews() const;

private:
    void createSwapchain();
    void createImageViews();

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    static uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities);

private:
    const VulkanDevice& device;
    const VulkanWindow& window;

    vk::raii::SwapchainKHR swapChain = nullptr;
    
    std::vector<vk::Image>           swapChainImages;
    vk::SurfaceFormatKHR             swapChainSurfaceFormat;
    vk::Extent2D                     swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;
};