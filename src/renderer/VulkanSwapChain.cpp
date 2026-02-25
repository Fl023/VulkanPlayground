#include "VulkanSwapchain.hpp"


VulkanSwapChain::VulkanSwapChain(const VulkanDevice& device, const VulkanWindow& window)
    : device(device), window(window)
{
    createSwapchain();
    createImageViews();
}

void VulkanSwapChain::recreate()
{
    int width = 0, height = 0;
    window.getFramebufferSize(width, height);
    
    // Pause if minimized
    while (width == 0 || height == 0) {
        window.getFramebufferSize(width, height);
        window.waitEvents();
    }

    // Cleanup old resources before recreating
    swapChainImageViews.clear();
    // Releasing the RAII handle destroys the old swapchain
    swapChain = nullptr; 

    createSwapchain();
    createImageViews();
}

const vk::raii::SwapchainKHR& VulkanSwapChain::getSwapChain() const { return swapChain; }
const std::vector<vk::Image>& VulkanSwapChain::getImages() const { return swapChainImages; }
const vk::SurfaceFormatKHR& VulkanSwapChain::getSurfaceFormat() const { return swapChainSurfaceFormat; }
const vk::Extent2D& VulkanSwapChain::getExtent() const { return swapChainExtent; }
const std::vector<vk::raii::ImageView>& VulkanSwapChain::getImageViews() const { return swapChainImageViews; }

vk::SurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && 
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR VulkanSwapChain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    window.getFramebufferSize(width, height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t VulkanSwapChain::chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities)
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

void VulkanSwapChain::createSwapchain()
{
    auto surfaceCapabilities = device.getPhysicalDevice().getSurfaceCapabilitiesKHR(*device.getSurface());
    auto availableFormats = device.getPhysicalDevice().getSurfaceFormatsKHR(*device.getSurface());
    auto availablePresentModes = device.getPhysicalDevice().getSurfacePresentModesKHR(*device.getSurface());

    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);
    
    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface = *device.getSurface(),
        .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(availablePresentModes),
        .clipped = true 
    };

    swapChain = vk::raii::SwapchainKHR(device.getDevice(), swapChainCreateInfo);
    
    swapChainImages = swapChain.getImages();
}

void VulkanSwapChain::createImageViews()
{
    assert(swapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo{ 
        .viewType = vk::ImageViewType::e2D, 
        .format = swapChainSurfaceFormat.format, 
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor, 
            .baseMipLevel = 0, 
            .levelCount = 1, 
            .baseArrayLayer = 0, 
            .layerCount = 1
        } 
    };

    for (auto& image : swapChainImages)
    {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device.getDevice(), imageViewCreateInfo);
    }
}