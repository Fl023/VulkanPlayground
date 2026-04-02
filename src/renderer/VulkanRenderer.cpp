#include "VulkanRenderer.hpp"
#include "VulkanHelpers.hpp"
#include <iostream>

VulkanRenderer::VulkanRenderer(VulkanWindow& window)
    : window(window),
    context(),
    m_Device(context, window),
    swapChain(m_Device, window),
    m_StandardAllocator(
        m_Device.getDevice(),
		50, // Enough for all camera sets, UI sets, compute sets (we need MAX_FRAMES_IN_FLIGHT sets per frame for the camera UBOs)
		{ vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 50) }, // The global UBO sets are the only ones we will allocate from this pool, so we only need uniform buffers in this allocator
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
    ),
    m_BindlessAllocator(
        m_Device.getDevice(),
		1, // just one big pool that we never free, since we only add more textures to the bindless array and never remove them.
		{ vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_TEXTURES * 2) }, // We need combined image samplers for the bindless array, and we allocate both textures and cubemaps from this pool, so we need enough for MAX_BINDLESS_TEXTURES of each (hence the *2)
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind
    )
{
    createGeneralDescriptorSetLayoutsAndSets();

    createDefaultTexture();

    createColorResources();
    createDepthResources();
    createSyncObjects();
}

VulkanRenderer::~VulkanRenderer()
{
    try {
        m_Device.getDevice().waitIdle();
    }
    catch (...) {
        std::cerr << "Error waiting for device idle in Renderer destructor." << std::endl;
    }
}

// ==========================================
// AAA FRAME LIFECYCLE
// ==========================================

bool VulkanRenderer::BeginFrame(const RenderView& view)
{
    // Wait for the previous frame's GPU execution to complete
    auto fenceResult = m_Device.getDevice().waitForFences(*frames[frameIndex].inFlightFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }

    // Process all VRAM cleanups for resources destroyed by the frontend last frame
    for (auto& func : m_DeletionQueues[frameIndex]) {
        func();
    }
    m_DeletionQueues[frameIndex].clear();

    vk::Semaphore acquireSemaphore = *frames[frameIndex].presentCompleteSemaphore;
    auto [result, imageIndex] = swapChain.getSwapChain().acquireNextImage(UINT64_MAX, acquireSemaphore, nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapchainResources();
        return false;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_CurrentImageIndex = imageIndex;

    // Dumb Byte Upload: Instantly sync the frontend's GlobalFrameData to the GPU
    memcpy(frames[frameIndex].uniformBuffer->getMappedData(), &view.FrameData, sizeof(GlobalFrameData));

    m_Device.getDevice().resetFences(*frames[frameIndex].inFlightFence);

    frames[frameIndex].commandBuffer.reset();
    vk::CommandBufferBeginInfo beginInfo{};
    frames[frameIndex].commandBuffer.begin(beginInfo);

    return true;
}

void VulkanRenderer::ExecuteRenderGraph(RenderGraph& renderGraph, const RenderView& view)
{
    // 1. Compile the graph to automatically generate all required memory transitions
    renderGraph.Compile();

    auto& cb = frames[frameIndex].commandBuffer;
    RGCommandList cmdListWrapper(cb);
    cmdListWrapper.SetGlobalState(*frames[frameIndex].globalSet->GetSet(), *m_BindlessDescriptorSet->GetSet());

    // Provide the wrapper with the global descriptors so it can bind them automatically later
    // cmdListWrapper.SetGlobalState(*frames[frameIndex].cameraSet, *bindlessDescriptorSet);

    for (const auto& pass : renderGraph.GetPasses())
    {
        // ==========================================
        // STEP A: AUTOMATIC LAYOUT TRANSITIONS
        // ==========================================
        for (const RGTransition& transition : pass.RequiredTransitions)
        {
            RGPhysicalResource* physImage = renderGraph.GetPhysicalImage(transition.Handle);
            if (physImage) {
                VulkanSyncInfo srcSync = GetSyncInfoFromLayout(transition.OldLayout);
                VulkanSyncInfo dstSync = GetSyncInfoFromLayout(transition.NewLayout);
                const RGTextureDesc& desc = renderGraph.GetVirtualTextures()[transition.Handle];
                vk::ImageAspectFlags aspectFlags = GetAspectFlagsFromFormat(desc.Format);

                m_Device.transitionImageLayout(
                    cb, physImage->Image,
                    transition.OldLayout, transition.NewLayout, 1,
                    srcSync.stageMask, dstSync.stageMask,
                    srcSync.accessMask, dstSync.accessMask,
                    aspectFlags
                );
            }
        }

        // ==========================================
        // STEP B: DYNAMIC RENDERING ATTACHMENT SETUP
        // ==========================================
        std::vector<vk::RenderingAttachmentInfo> colorAttachments;
        vk::RenderingAttachmentInfo depthAttachment{};
        bool hasDepth = false;

        vk::Extent2D passExtent = swapChain.getExtent();

        // 1. Pre-pass: Check if this RenderPass explicitly outputted the Viewport Resolve Image
        RGPhysicalResource* resolveTargetForThisPass = nullptr;
        for (const RGResourceUsage& output : pass.Outputs) {
            RGPhysicalResource* physImage = renderGraph.GetPhysicalImage(output.Handle);
            if (physImage && physImage->Image == GetViewportResolveImage()) {
                resolveTargetForThisPass = physImage;
                break;
            }
        }

        // 2. Build the actual attachments
        for (const RGResourceUsage& output : pass.Outputs) {
            RGPhysicalResource* physImage = renderGraph.GetPhysicalImage(output.Handle);
            if (!physImage) continue;

            // SKIP THE RESOLVE IMAGE HERE! We don't want it as a primary color attachment.
            if (physImage == resolveTargetForThisPass) continue;

            const RGTextureDesc& desc = renderGraph.GetVirtualTextures()[output.Handle];
            vk::ImageAspectFlags aspect = GetAspectFlagsFromFormat(desc.Format);

            if (physImage->Image == GetViewportColorImage() || physImage->Image == GetViewportDepthImage()) {
                passExtent = { m_ViewportTarget->GetWidth(), m_ViewportTarget->GetHeight() };
            }

            if (aspect & vk::ImageAspectFlagBits::eColor) {
                vk::RenderingAttachmentInfo colorAtt{
                    .imageView = physImage->ImageView,
                    .imageLayout = output.Layout,
                    .loadOp = vk::AttachmentLoadOp::eClear,
                    .storeOp = vk::AttachmentStoreOp::eStore,
                    .clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
                };

                // If this is the MSAA Color Image AND we found a resolve target in the Graph's outputs, link them!
                if (physImage->Image == GetViewportColorImage() && resolveTargetForThisPass) {
                    // Usually you use eAverage or eResolve depending on your Vulkan limits, eAverage is standard for color
                    colorAtt.resolveMode = vk::ResolveModeFlagBits::eAverage;
                    colorAtt.resolveImageView = resolveTargetForThisPass->ImageView;
                    colorAtt.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
                }

                colorAttachments.push_back(colorAtt);
            }
            else if (aspect & vk::ImageAspectFlagBits::eDepth) {
                depthAttachment = vk::RenderingAttachmentInfo{
                    .imageView = physImage->ImageView,
                    .imageLayout = output.Layout,
                    .loadOp = vk::AttachmentLoadOp::eClear,
                    .storeOp = vk::AttachmentStoreOp::eDontCare, // We don't need to save MSAA depth after the pass!
                    .clearValue = vk::ClearDepthStencilValue(1.0f, 0)
                };
                hasDepth = true;
            }
        }

        // Only start rendering if the pass actually writes to something!
        if (!colorAttachments.empty() || hasDepth) {
            vk::RenderingInfo renderingInfo{
                .renderArea = { {0, 0}, passExtent },
                .layerCount = 1,
                .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
                .pColorAttachments = colorAttachments.data(),
                .pDepthAttachment = hasDepth ? &depthAttachment : nullptr
            };

            cb.beginRendering(renderingInfo);

            vk::Viewport viewport{ 0.0f, (float)passExtent.height, (float)passExtent.width, -(float)passExtent.height, 0.0f, 1.0f };
            cb.setViewport(0, viewport);

            vk::Rect2D scissor{ {0, 0}, passExtent };
            cb.setScissor(0, scissor);
        }

        // ==========================================
        // STEP C: FRONTEND EXECUTION
        // ==========================================
        pass.Execute(cmdListWrapper, view);

        if (!colorAttachments.empty() || hasDepth) {
            cb.endRendering();
        }
    }
}

void VulkanRenderer::EndFrame()
{
    m_Device.transitionImageLayout(
        frames[frameIndex].commandBuffer,                   // Your RAII command buffer
        GetCurrentSwapchainImage(),                         // The raw vk::Image
        vk::ImageLayout::eColorAttachmentOptimal,           // oldLayout
        vk::ImageLayout::ePresentSrcKHR,                    // newLayout
        1,                                                  // mipLevels
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStageMask (Wait for rendering to finish)
        vk::PipelineStageFlagBits2::eBottomOfPipe,          // dstStageMask (Push to the very end of the pipeline)
        vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask (We wrote color data)
        vk::AccessFlagBits2::eNone,                         // dstAccessMask (No further CPU/GPU access needed)
        vk::ImageAspectFlagBits::eColor,                    // imageAspectFlags
        1                                                   // layerCount
    );

    vk::CommandBuffer cb = *frames[frameIndex].commandBuffer;
    cb.end();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::Semaphore acquireSemaphore = *frames[frameIndex].presentCompleteSemaphore;
    vk::Semaphore renderFinishedSemaphore = *renderFinishedSemaphores[m_CurrentImageIndex];

    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &acquireSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &cb,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinishedSemaphore
    };

    m_Device.getQueue().submit(submitInfo, *frames[frameIndex].inFlightFence);
}

void VulkanRenderer::Present()
{
    vk::Semaphore renderFinishedSemaphore = *renderFinishedSemaphores[m_CurrentImageIndex];
    const vk::SwapchainKHR sc = *swapChain.getSwapChain();

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &sc,
        .pImageIndices = &m_CurrentImageIndex
    };

    vk::Result result;
    try {
        result = m_Device.getQueue().presentKHR(presentInfoKHR);
    }
    catch (const vk::OutOfDateKHRError&) {
        result = vk::Result::eErrorOutOfDateKHR;
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapchainResources();
    }
    else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}


// ==========================================
// INITIALIZATION & INTERNAL HELPERS
// ==========================================

void VulkanRenderer::recordImGuiCommands(vk::CommandBuffer commandBuffer)
{
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data) {
        ImGui_ImplVulkan_RenderDrawData(draw_data, static_cast<VkCommandBuffer>(commandBuffer));
    }
}

void VulkanRenderer::createGeneralDescriptorSetLayoutsAndSets()
{
	// 1. Setup the Global UBO Layout (Set 0) for per-frame data like camera matrices, time, etc.
    vk::DescriptorSetLayoutBinding globalBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eAllGraphics, nullptr);
    vk::DescriptorSetLayoutCreateInfo globalLayoutInfo{ .bindingCount = 1, .pBindings = &globalBinding };

    // 2. Setup the Global Bindless Layout (Set 1)
    std::array<vk::DescriptorSetLayoutBinding, 2> bindlessBindings = {
        // Binding 0: 2D Textures
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_TEXTURES, vk::ShaderStageFlagBits::eAllGraphics, nullptr),
        // Binding 1: Cubemaps
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_TEXTURES, vk::ShaderStageFlagBits::eAllGraphics, nullptr)
    };

    // VULKAN RULE: Only the last binding (Binding 1) can be VariableDescriptorCount!
    std::array<vk::DescriptorBindingFlags, 2> bindlessFlags = {
        vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    };

    vk::DescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{
        .bindingCount = static_cast<uint32_t>(bindlessFlags.size()),
        .pBindingFlags = bindlessFlags.data()
    };

    vk::DescriptorSetLayoutCreateInfo bindlessLayoutInfo{
        .pNext = &extendedInfo,
        .flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
        .bindingCount = static_cast<uint32_t>(bindlessBindings.size()),
        .pBindings = bindlessBindings.data()
    };

	// 3. Build the Pipeline Layout with both Set 0 and Set 1
    m_GeneralPipelineSignature.AddSetLayout(0, vk::raii::DescriptorSetLayout(m_Device.getDevice(), globalLayoutInfo));
    m_GeneralPipelineSignature.AddSetLayout(1, vk::raii::DescriptorSetLayout(m_Device.getDevice(), bindlessLayoutInfo));
    m_GeneralPipelineSignature.AddPushConstantRange(sizeof(PushConstants), vk::ShaderStageFlagBits::eAllGraphics, 0);
    m_GeneralPipelineSignature.Build(m_Device.getDevice());

	// 4. Now we can create the per-frame camera sets from the general layout of Set 0
    frames.reserve(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        frames.emplace_back(m_Device, m_GeneralPipelineSignature.GetSetLayout(0), sizeof(GlobalFrameData), &m_StandardAllocator);
	}

	// 5. Finally, create the big bindless descriptor set from the general layout of Set 1
    m_BindlessDescriptorSet = std::make_unique<DescriptorSetInstance>(
        m_Device.getDevice(), m_GeneralPipelineSignature.GetSetLayout(1), &m_BindlessAllocator
    );
}

void VulkanRenderer::createDefaultTexture()
{
    uint32_t whitePixel = 0xFFFFFFFF;
    m_DefaultTexture = std::make_unique<Texture>(m_Device, 1, 1, &whitePixel);
    AddTextureToBindlessArray(m_DefaultTexture.get());
}

void VulkanRenderer::AddTextureToBindlessArray(Texture* texture)
{
    uint32_t indexToUse;
    uint32_t targetBinding = texture->IsCubemap() ? 1 : 0;

    if (texture->IsCubemap())
    {
        if (!m_FreeCubemapIndices.empty()) {
            indexToUse = m_FreeCubemapIndices.back();
            m_FreeCubemapIndices.pop_back();
        }
        else {
            if (currentCubemapIndex >= MAX_BINDLESS_TEXTURES) throw std::runtime_error("Exceeded maximum bindless cubemaps!");
            indexToUse = currentCubemapIndex++;
        }
    }
    else
    {
        if (!m_FreeTextureIndices.empty()) {
            indexToUse = m_FreeTextureIndices.back();
            m_FreeTextureIndices.pop_back();
        }
        else {
            if (currentTextureIndex >= MAX_BINDLESS_TEXTURES) throw std::runtime_error("Exceeded maximum bindless textures!");
            indexToUse = currentTextureIndex++;
        }
    }

    texture->SetBindlessIndex(indexToUse);
    vk::DescriptorImageInfo imageInfo = texture->GetDescriptorInfo();

    m_BindlessDescriptorSet->WriteImage(
        targetBinding, 
        imageInfo,
		indexToUse
	);
}

void VulkanRenderer::FreeBindlessIndex(uint32_t index, bool isCubemap) 
{ 
    isCubemap ? m_FreeCubemapIndices.push_back(index) : m_FreeTextureIndices.push_back(index);
}
void VulkanRenderer::SubmitToDeletionQueue(std::function<void()>&& function) { m_DeletionQueues[frameIndex].push_back(std::move(function)); }

void VulkanRenderer::InitViewport()
{
    m_ViewportTarget.emplace(
        m_Device,
        static_cast<uint32_t>(window.getWidth()),
        static_cast<uint32_t>(window.getHeight()),
        swapChain.getSurfaceFormat().format
    );
}

void VulkanRenderer::createSyncObjects()
{
    renderFinishedSemaphores.clear();
    for (size_t i = 0; i < swapChain.getImages().size(); i++) {
        renderFinishedSemaphores.emplace_back(m_Device.getDevice(), vk::SemaphoreCreateInfo());
    }
}

void VulkanRenderer::createDepthResources()
{
    depthImage.emplace(m_Device, swapChain.getExtent().width, swapChain.getExtent().height, 1, m_Device.getMsaaSamples(),
        vk::Format::eD32Sfloat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth);
}

void VulkanRenderer::createColorResources()
{
    colorImage.emplace(m_Device, swapChain.getExtent().width, swapChain.getExtent().height, 1, m_Device.getMsaaSamples(),
        swapChain.getSurfaceFormat().format, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
}

void VulkanRenderer::recreateSwapchainResources()
{
    m_Device.getDevice().waitIdle();
    swapChain.recreate();
    createColorResources();
    createDepthResources();
    createSyncObjects();
}