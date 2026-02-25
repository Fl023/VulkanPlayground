#include "VulkanRenderer.hpp"


VulkanRenderer::VulkanRenderer()
    : window(),
      context(),
      device(context, window),
      swapChain(device, window),
      graphicsPipeline(device, swapChain),
      materialAllocator(device.getDevice(), 1000, { {vk::DescriptorType::eCombinedImageSampler, 1} })
{
    window.setUserPointer(this);

    vk::DeviceSize uboSize = sizeof(UniformBufferObject);

    frames.reserve(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        frames.emplace_back(device, graphicsPipeline.getGlobalDescriptorSetLayout(), uboSize);
    }

    createColorResources();
    createDepthResources();

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
        recreateSwapchainResources();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

	// Update uniform buffer with the current image index before recording commands
    updateUniformBuffer(frameIndex, scene);

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
        recreateSwapchainResources();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Advance frame index
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

std::shared_ptr<Material> VulkanRenderer::createMaterial(std::shared_ptr<Texture> texture)
{
    vk::raii::DescriptorSet set = materialAllocator.allocate(graphicsPipeline.getMaterialDescriptorSetLayout());

    vk::DescriptorImageInfo imageInfo = texture->GetDescriptorInfo();

    vk::WriteDescriptorSet descriptorWrite{
        .dstSet = *set,
        .dstBinding = 0, // Binding 0 in Set 1
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo
    };

    device.getDevice().updateDescriptorSets(descriptorWrite, {});

    return std::make_shared<Material>(texture, std::move(set));
}

void VulkanRenderer::createSyncObjects()
{
    renderFinishedSemaphores.clear();
    // Erstelle genau ein Semaphore pro physischem Swapchain-Bild
    for (size_t i = 0; i < swapChain.getImages().size(); i++) {
        renderFinishedSemaphores.emplace_back(device.getDevice(), vk::SemaphoreCreateInfo());
    }
}

void VulkanRenderer::createDepthResources()
{
    depthImage.emplace(device, 
        swapChain.getExtent().width, 
        swapChain.getExtent().height, 
        1,
        device.getMsaaSamples(),
        vk::Format::eD32Sfloat, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eDepthStencilAttachment, 
        vk::MemoryPropertyFlagBits::eDeviceLocal, 
        vk::ImageAspectFlagBits::eDepth);
}

void VulkanRenderer::createColorResources()
{
    colorImage.emplace(device,
        swapChain.getExtent().width,
        swapChain.getExtent().height,
        1,
        device.getMsaaSamples(),
        swapChain.getSurfaceFormat().format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );
}

void VulkanRenderer::recreateSwapchainResources()
{
    device.getDevice().waitIdle();
    swapChain.recreate();
    createColorResources();
    createDepthResources();
    createSyncObjects();
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage, Scene& scene) {
    UniformBufferObject ubo{};

    auto view = scene.m_Registry.view<TransformComponent, CameraComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& camComp = view.get<CameraComponent>(entity);

        if (camComp.Primary) {
            // 1. Viewport an Swapchain anpassen
            camComp.SceneCamera.SetViewportSize(swapChain.getExtent().width, swapChain.getExtent().height);

            // 2. View-Matrix basierend auf dem Transform der Entity berechnen
            camComp.SceneCamera.RecalculateViewMatrix(transform.GetTransform());

            // 3. Matrizen ins UBO kopieren
            ubo.view = camComp.SceneCamera.GetViewMatrix();
            ubo.proj = camComp.SceneCamera.GetProjectionMatrix();
            break;
        }
    }

    memcpy(frames[currentImage].uniformBuffer->getMappedData(), &ubo, sizeof(ubo));
}



void VulkanRenderer::recordCommandBuffer(uint32_t imageIndex, Scene& scene) {
    auto& commandBuffer = frames[frameIndex].commandBuffer;
    
    // Begin command buffer
    vk::CommandBufferBeginInfo beginInfo{};
    commandBuffer.begin(beginInfo);

    device.transitionImageLayout(
        commandBuffer,
        swapChain.getImages()[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        1,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageAspectFlagBits::eColor
	);

    device.transitionImageLayout(
        commandBuffer,
        colorImage->getImage(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        1,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageAspectFlagBits::eColor);

    device.transitionImageLayout(
        commandBuffer,
        depthImage->getImage(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        1,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::ImageAspectFlagBits::eDepth
    );

    // Dynamic Rendering Setup
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    
    vk::RenderingAttachmentInfo colorAttachment = {
        .imageView = colorImage->getImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = swapChain.getImageViews()[imageIndex],
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor 
    };

    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = depthImage->getImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth 
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = { 0, 0 }, .extent = swapChain.getExtent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo);

    // Bind Pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getGraphicsPipeline());

    // Bind Descriptor Sets
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        graphicsPipeline.getPipelineLayout(),
        0,
        *frames[frameIndex].cameraSet,
        {}
    );

    // Dynamic State: Viewport & Scissor
    vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapChain.getExtent().width), static_cast<float>(swapChain.getExtent().height), 0.0f, 1.0f);
    commandBuffer.setViewport(0, viewport);

    vk::Rect2D scissor({ 0, 0 }, swapChain.getExtent());
    commandBuffer.setScissor(0, scissor);

    auto view = scene.m_Registry.view<MeshComponent, TransformComponent, MaterialComponent>();

    vk::DescriptorSet lastBoundMaterialSet = nullptr;

    for (auto entity : view) {
        auto& meshComp = view.get<MeshComponent>(entity);
        auto& transformComp = view.get<TransformComponent>(entity);
        auto& matComp = view.get<MaterialComponent>(entity);

        if (meshComp.MeshAsset) {
            glm::mat4 modelMatrix = transformComp.GetTransform();

            // Send push constant to gpu
            commandBuffer.pushConstants<glm::mat4>(
                graphicsPipeline.getPipelineLayout(),
                vk::ShaderStageFlagBits::eVertex,
                0,
                modelMatrix
            );

            if (matComp.materialAsset) {
                vk::DescriptorSet currentSet = *matComp.materialAsset->getDescriptorSet();

                if (currentSet != lastBoundMaterialSet) {
                    commandBuffer.bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics,
                        graphicsPipeline.getPipelineLayout(),
                        1,
                        currentSet,
                        {}
                    );
                    lastBoundMaterialSet = currentSet; // Zustand merken
                }
            }

            // Bind Vertex & Index Buffer
            commandBuffer.bindVertexBuffers(0, *meshComp.MeshAsset->getVertexBuffer(), { 0 });
            commandBuffer.bindIndexBuffer(*meshComp.MeshAsset->getIndexBuffer(), 0, vk::IndexType::eUint16);

            commandBuffer.drawIndexed(meshComp.MeshAsset->getIndexCount(), 1, 0, 0, 0);
        }
    }

    commandBuffer.endRendering();

    device.transitionImageLayout(
        commandBuffer,
        swapChain.getImages()[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        1,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::ImageAspectFlagBits::eColor
	);

    commandBuffer.end();
}