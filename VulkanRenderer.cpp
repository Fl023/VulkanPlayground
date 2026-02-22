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
    updateUniformBuffer(frameIndex, scene);

    auto materialView = scene.m_Registry.view<MaterialComponent>();
    if (materialView.begin() != materialView.end()) {
        auto entity = materialView.front(); // Wir nehmen das erste Objekt mit Material
        auto& matComp = materialView.get<MaterialComponent>(entity);

        if (matComp.albedoTexture) {
            // Infos aus der Textur holen (View + Sampler)
            vk::DescriptorImageInfo imageInfo = matComp.albedoTexture->GetDescriptorInfo();

            // Schreib-Befehl für Binding 1 (unser Sampler im Shader)
            vk::WriteDescriptorSet descriptorWrite{
                .dstSet = *frames[frameIndex].descriptorSet,
                .dstBinding = 1, // Muss mit layout(binding = 1) im Fragment Shader übereinstimmen!
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &imageInfo
            };

            // An die GPU schicken
            device.getDevice().updateDescriptorSets(descriptorWrite, {});
        }
    }

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

    auto meshView = scene.m_Registry.view<TransformComponent, MeshComponent>();
    if (meshView.begin() != meshView.end()) {
        auto entity = meshView.front(); // Wir nehmen einfach das erste Objekt
        ubo.model = meshView.get<TransformComponent>(entity).GetTransform();
    }
    else {
        ubo.model = glm::mat4(1.0f);
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
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite
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

    device.transitionImageLayout(
        commandBuffer,
        swapChain.getImages()[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {}
	);

    commandBuffer.end();
}