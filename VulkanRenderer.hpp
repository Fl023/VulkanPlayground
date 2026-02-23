#pragma once

#include "VulkanContext.hpp"
#include "VulkanWindow.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "VulkanFrame.hpp"
#include "VulkanVertex.hpp"
#include "VulkanBuffer.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Components.hpp"

class VulkanRenderer
{
public:
    VulkanRenderer();
    ~VulkanRenderer();

    const VulkanContext& getContext() const;
	const VulkanDevice& getDevice() const { return device; }
    const VulkanWindow& getWindow() const;

    void drawFrame(Scene& scene);

    std::shared_ptr<Material> createMaterial(std::shared_ptr<Texture> texture);

    // Public variable accessed by the Window callback
    bool framebufferResized = false;

private:
    void createSyncObjects();

    void updateUniformBuffer(uint32_t currentImage, Scene& scene);

    void recordCommandBuffer(uint32_t imageIndex, Scene& scene);

private:
    VulkanWindow  window;
    VulkanContext context;
    VulkanDevice  device;
    VulkanSwapChain swapChain;
    VulkanGraphicsPipeline graphicsPipeline;

    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VulkanFrame> frames;
    uint32_t frameIndex = 0;

    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

    DescriptorAllocator materialAllocator;
};