#pragma once

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanFrame.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"
#include "VulkanRenderTarget.hpp"
#include "VulkanDescriptorAllocator.hpp"
#include "scene/Texture.hpp"
#include "SceneRenderer.hpp"

#include "RenderGraph.hpp"
#include "scene/RenderView.hpp"
#include "RGCommandList.hpp"

class VulkanRenderer
{
public:
    VulkanRenderer(VulkanWindow& window);
    ~VulkanRenderer();

    // --- CORE GETTERS ---
    const VulkanContext& getContext() const { return context; }
    const VulkanDevice& getDevice() const { return m_Device; }
    const VulkanWindow& getWindow() const { return window; }
	const VulkanSwapChain& getSwapChain() const { return swapChain; }

    // --- AAA LIFECYCLE ---
    bool BeginFrame(const RenderView& view);
    void ExecuteRenderGraph(RenderGraph& renderGraph, const RenderView& view);
    void EndFrame();
    void Present();

    void recordImGuiCommands(vk::CommandBuffer commandBuffer);

    // --- BINDLESS RESOURCE MANAGEMENT ---
    void AddTextureToBindlessArray(Texture* texture);
    void FreeBindlessIndex(uint32_t index, bool isCubemap = true);
    void SubmitToDeletionQueue(std::function<void()>&& function);

    // --- VIEWPORT / GRAPH UTILS ---
    void InitViewport();
    void DestroyViewport() { m_ViewportTarget.reset(); }
    vk::DescriptorSet GetViewportTextureID() const { return m_ViewportTarget->GetImGuiTextureID(); }
    void ResizeViewport(uint32_t width, uint32_t height) { m_ViewportTarget->Resize(width, height); }

    // Viewport Color (MSAA)
    vk::Image GetViewportColorImage() { return *m_ViewportTarget->GetColorImage().getImage(); }
    vk::ImageView GetViewportColorImageView() { return *m_ViewportTarget->GetColorImage().getImageView(); }

    // Viewport Resolve (1-Sample)
    vk::Image GetViewportResolveImage() { return *m_ViewportTarget->GetResolveImage().getImage(); }
    vk::ImageView GetViewportResolveImageView() { return *m_ViewportTarget->GetResolveImage().getImageView(); }

    // Viewport Depth
    vk::Image GetViewportDepthImage() { return *m_ViewportTarget->GetDepthImage().getImage(); }
    vk::ImageView GetViewportDepthImageView() { return *m_ViewportTarget->GetDepthImage().getImageView(); }

    // Swapchain
    vk::Format GetSwapchainFormat() const { return swapChain.getSurfaceFormat().format; }
    // Assuming swapChain.getImages() returns std::vector<vk::Image>
    vk::Image GetCurrentSwapchainImage() { return swapChain.getImages()[m_CurrentImageIndex]; }
    vk::ImageView GetCurrentSwapchainImageView() { return swapChain.getImageViews()[m_CurrentImageIndex]; }

	PipelineSignature* GetGeneralPipelineSignature() { return &m_GeneralPipelineSignature; }

    bool framebufferResized = false;

private:
	// we create a basic descriptor set layout that we will use for most of the shaders.
	// This way we can just bind the same descriptor set for all draw calls even across different shaders.
	// Only for a few special cases like compute shaders, we will create separate layouts and bind different sets.
	// But these layouts will be created by the spirV reflection system, not manually like this one.
    void createGeneralDescriptorSetLayoutsAndSets();

    // each frame in flight needs an own global uniform buffer. The global uniform buffers get updated every frame,
    // so we can't update the descriptor set while a shader uses it.
	// since the bindless descriptor sets don't get completely overwritten every frame like the global sets do, 
    // we can create them once and then just update the relevant bindings when we add new textures.
	// To ensure that we get no dataraces when updating the bindless descriptor set while a shader is using it, 
	// we queue delete operations for the bindless descriptor set updates and execute them at the beginning of the next time this frame gets rendered.

    void createDefaultTexture();
    void createColorResources();
    void createDepthResources();
    void createSyncObjects();
    void recreateSwapchainResources();

private:
    VulkanWindow& window;
    VulkanContext context;
    VulkanDevice  m_Device;
    VulkanSwapChain swapChain;

    // The Fast-Path Allocator (For static UBOs, Post-Processing, Compute)
    DescriptorAllocator m_StandardAllocator;

    // The Bindless Allocator (Only for the giant dynamic arrays)
    DescriptorAllocator m_BindlessAllocator;

	// The general pipeline signature that most shaders will use. It contains the global set and the bindless set.
	PipelineSignature m_GeneralPipelineSignature;

	// The global camera set is contained in the frame data,
	// and the bindless set is stored here in the renderer since it is shared across all frames and gets updated less frequently.
	std::unique_ptr<DescriptorSetInstance> m_BindlessDescriptorSet;

    std::optional<VulkanImage> depthImage;
    std::optional<VulkanImage> colorImage;

    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VulkanFrame> frames;
    uint32_t frameIndex = 0;
    uint32_t m_CurrentImageIndex = 0;

    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

    static constexpr uint32_t MAX_BINDLESS_TEXTURES = 1000;
    std::vector<uint32_t> m_FreeTextureIndices;
    uint32_t currentTextureIndex = 0;

    uint32_t currentCubemapIndex = 0;
    std::vector<uint32_t> m_FreeCubemapIndices;
    std::unique_ptr<Texture> m_DefaultTexture;

    std::optional<RenderTarget> m_ViewportTarget;

    std::vector<std::function<void()>> m_DeletionQueues[MAX_FRAMES_IN_FLIGHT];
};