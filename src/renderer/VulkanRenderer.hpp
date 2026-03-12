#pragma once

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "VulkanFrame.hpp"
#include "VulkanVertex.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"
#include "scene/Mesh.hpp"
#include "scene/Scene.hpp"
#include "scene/Components.hpp"
#include "scene/AssetManager.hpp"
#include "ImGuiLayer.hpp"

class VulkanRenderer
{
public:
    VulkanRenderer(VulkanWindow& window);
    ~VulkanRenderer();

    const VulkanContext& getContext() const;
	const VulkanDevice& getDevice() const { return device; }
    const VulkanWindow& getWindow() const;

    void drawFrame(Scene& scene, AssetManager& assetManager);
    void beginUI();

    void AddTextureToBindlessArray(Texture* texture);
    void FreeBindlessIndex(uint32_t index);

    void SubmitToDeletionQueue(std::function<void()>&& function);

    // Public variable accessed by the Window callback
    bool framebufferResized = false;

private:
    void createSyncObjects();

    void createDepthResources();

    void createColorResources();

    void recreateSwapchainResources();

    void updateUniformBuffer(uint32_t currentImage, Scene& scene);

    void recordCommandBuffer(uint32_t imageIndex, Scene& scene, AssetManager& assetManager);

    void createBindlessDescriptorSet();

    void createDefaultTexture();

private:
    VulkanWindow&  window;
    VulkanContext context;
    VulkanDevice  device;
    VulkanSwapChain swapChain;
    VulkanGraphicsPipeline graphicsPipeline;
    std::optional<VulkanImage> depthImage;
    std::optional<VulkanImage> colorImage;

    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VulkanFrame> frames;
    uint32_t frameIndex = 0;

    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

    vk::raii::DescriptorPool bindlessPool = nullptr;
    vk::raii::DescriptorSet bindlessDescriptorSet = nullptr;

    static constexpr uint32_t MAX_BINDLESS_TEXTURES = 1000;
    std::vector<uint32_t> m_FreeTextureIndices;
    uint32_t currentTextureIndex = 0;
    std::unique_ptr<Texture> m_DefaultTexture;
    std::unique_ptr<ImGuiLayer> imGuiLayer;

    std::vector<std::function<void()>> m_DeletionQueues[MAX_FRAMES_IN_FLIGHT];
};