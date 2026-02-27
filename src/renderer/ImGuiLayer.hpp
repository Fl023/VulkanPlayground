#pragma once

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

class ImGuiLayer
{
public:
    ImGuiLayer(const VulkanDevice& device, const VulkanSwapChain& swapChain);
    ~ImGuiLayer();

    // Startet den neuen ImGui-Frame (sammelt Input etc.)
    void beginFrame();

    // Nimmt die fertigen UI-Daten und schreibt sie in den CommandBuffer
    void recordImGuiCommands(const vk::raii::CommandBuffer& commandBuffer);

private:
    const VulkanDevice& m_Device;
    VkFormat m_ColorFormat;
};