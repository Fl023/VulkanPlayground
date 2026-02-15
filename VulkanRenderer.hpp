#pragma once

#include "VulkanContext.hpp"
#include "VulkanWindow.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "VulkanFrame.hpp"

class VulkanRenderer
{
public:
    VulkanRenderer();
    ~VulkanRenderer();

    const VulkanContext& getContext() const;
    const VulkanWindow& getWindow() const;

    void drawFrame();

    // Public variable accessed by the Window callback
    bool framebufferResized = false;

private:
    void recordCommandBuffer(uint32_t imageIndex);

    void transition_image_layout(
        uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask
    );

private:
    VulkanWindow  window;
    VulkanContext context;
    VulkanDevice  device;
    VulkanSwapChain swapChain;
    VulkanGraphicsPipeline graphicsPipeline;

    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VulkanFrame> frames;
    uint32_t frameIndex = 0;
};