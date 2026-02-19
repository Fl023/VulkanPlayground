#include "VulkanRenderer.hpp"


VulkanRenderer::VulkanRenderer()
    : window(),
      context(),
      device(context, window),
      swapChain(device, window),
      graphicsPipeline(device, swapChain)
{
    window.setUserPointer(this);

    vk::DeviceSize uboSize = sizeof(UniformBufferObject);

    frames.reserve(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        frames.emplace_back(device, graphicsPipeline.getDescriptorSetLayout(), uboSize);
    }

	createSyncObjects();
}

VulkanRenderer::~VulkanRenderer()
{
    try {
        device.getDevice().waitIdle();
    } catch (...) {
        std::cerr << "Error waiting for device idle in Renderer destructor." << std::endl;
    }
}

const VulkanContext& VulkanRenderer::getContext() const
{
    return context;
}

const VulkanWindow& VulkanRenderer::getWindow() const
{
    return window;
}

void VulkanRenderer::drawFrame(Scene& scene)
{
    // 1. Wait for the previous frame to finish
    auto fenceResult = device.getDevice().waitForFences(*frames[frameIndex].inFlightFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to wait for fence!");
    }

    // 2. Acquire an image from the swap chain
    vk::Semaphore acquireSemaphore = *frames[frameIndex].presentCompleteSemaphore;
    auto [result, imageIndex] = swapChain.getSwapChain().acquireNextImage(UINT64_MAX, acquireSemaphore, nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        swapChain.recreate();
        createSyncObjects();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

	// Update uniform buffer with the current image index before recording commands
    updateUniformBuffer(frameIndex);

    // Only reset the fence if we are actually going to submit work
    device.getDevice().resetFences(*frames[frameIndex].inFlightFence);

    // 3. Record commands
    frames[frameIndex].commandBuffer.reset();
    recordCommandBuffer(imageIndex, scene);

    // 4. Submit the recorded command buffer
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    
    vk::Semaphore renderFinishedSemaphore = *renderFinishedSemaphores[imageIndex];
    vk::CommandBuffer cb = *frames[frameIndex].commandBuffer;

    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &acquireSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &cb,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinishedSemaphore 
    };

    device.getQueue().submit(submitInfo, *frames[frameIndex].inFlightFence);

    // 5. Present the swap chain image
    const vk::SwapchainKHR sc = *swapChain.getSwapChain();

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &sc,
        .pImageIndices = &imageIndex 
    };

    try {
        result = device.getQueue().presentKHR(presentInfoKHR);
    } catch (const vk::OutOfDateKHRError&) {
        result = vk::Result::eErrorOutOfDateKHR;
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
    {
        framebufferResized = false;
        swapChain.recreate();
        createSyncObjects();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Advance frame index
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::createSyncObjects()
{
    renderFinishedSemaphores.clear();
    // Erstelle genau ein Semaphore pro physischem Swapchain-Bild
    for (size_t i = 0; i < swapChain.getImages().size(); i++) {
        renderFinishedSemaphores.emplace_back(device.getDevice(), vk::SemaphoreCreateInfo());
    }
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChain.getExtent().width) / static_cast<float>(swapChain.getExtent().height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(frames[currentImage].uniformBuffer->getMappedData(), &ubo, sizeof(ubo));
}

void VulkanRenderer::recordCommandBuffer(uint32_t imageIndex, Scene& scene) {
    auto& commandBuffer = frames[frameIndex].commandBuffer;
    
    // Begin command buffer
    vk::CommandBufferBeginInfo beginInfo{};
    commandBuffer.begin(beginInfo);

    // Transition: Undefined -> Color Attachment
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                         
        vk::AccessFlagBits2::eColorAttachmentWrite,                 
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,         
        vk::PipelineStageFlagBits2::eColorAttachmentOutput          
    );

    // Dynamic Rendering Setup
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = swapChain.getImageViews()[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = { 0, 0 }, .extent = swapChain.getExtent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo);

    // Bind Pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getGraphicsPipeline());

    // Bind Descriptor Sets
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        graphicsPipeline.getPipelineLayout(),
        0,
        *frames[frameIndex].descriptorSet,
        {}
    );

    auto view = scene.m_Registry.view<MeshComponent>();

    for (auto entity : view) {
        auto& meshComp = view.get<MeshComponent>(entity);

        if (meshComp.MeshAsset) {
            // Bind Vertex & Index Buffer
            commandBuffer.bindVertexBuffers(0, *meshComp.MeshAsset->getVertexBuffer(), { 0 });
            commandBuffer.bindIndexBuffer(*meshComp.MeshAsset->getIndexBuffer(), 0, vk::IndexType::eUint16);

            // Dynamic State: Viewport & Scissor
            vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapChain.getExtent().width), static_cast<float>(swapChain.getExtent().height), 0.0f, 1.0f);
            commandBuffer.setViewport(0, viewport);

            vk::Rect2D scissor({ 0, 0 }, swapChain.getExtent());
            commandBuffer.setScissor(0, scissor);

            commandBuffer.drawIndexed(meshComp.MeshAsset->getIndexCount(), 1, 0, 0, 0);
        }
    }

    commandBuffer.endRendering();

    // Transition: Color Attachment -> Present
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,             
        {},                                                     
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,     
        vk::PipelineStageFlagBits2::eBottomOfPipe               
    );

    commandBuffer.end();
}

void VulkanRenderer::transition_image_layout(
    uint32_t imageIndex,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask
) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChain.getImages()[imageIndex],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    frames[frameIndex].commandBuffer.pipelineBarrier2(dependencyInfo);
}