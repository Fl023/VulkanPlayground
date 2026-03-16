#pragma once

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDescriptorAllocator.hpp"
#include "scene/AssetHandle.hpp"

struct VulkanFrame {
	// Command Resources
	vk::raii::CommandPool commandPool = nullptr;
	vk::raii::CommandBuffer commandBuffer = nullptr;

	// Synchronization Primitives
	vk::raii::Semaphore presentCompleteSemaphore = nullptr; // GPU-GPU: Swapchain -> Render
	vk::raii::Fence inFlightFence = nullptr;               // GPU-CPU: GPU done -> CPU can reuse frame

	// UBO Member
	std::optional<VulkanBuffer> uniformBuffer;

	// Descriptor Resources
    DescriptorAllocator frameAllocator;
	vk::raii::DescriptorSet cameraSet = nullptr;
    vk::raii::DescriptorSet skyboxSet = nullptr;
    AssetHandle activeSkyboxHandle = INVALID_ASSET_HANDLE;

	VulkanFrame(const VulkanDevice& device, const vk::raii::DescriptorSetLayout& cameraLayout, const vk::raii::DescriptorSetLayout& skyboxLayout, vk::DeviceSize uboSize)
        : frameAllocator(device.getDevice(), 2, {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
            })
	{
        // 1. Command Pool & Buffer erstellen
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = device.getQueueIndex()
        };
        commandPool = vk::raii::CommandPool(device.getDevice(), poolInfo);

        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        commandBuffer = std::move(vk::raii::CommandBuffers(device.getDevice(), allocInfo).front());

        // 2. Synchronisationsobjekte
        presentCompleteSemaphore = vk::raii::Semaphore(device.getDevice(), vk::SemaphoreCreateInfo());
        inFlightFence = vk::raii::Fence(device.getDevice(), { .flags = vk::FenceCreateFlagBits::eSignaled });

        // 3. Uniform Buffer erstellen (HostVisible + HostCoherent für Persistent Mapping)
        uniformBuffer.emplace(
            device,
            uboSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        cameraSet = frameAllocator.allocate(cameraLayout);
        skyboxSet = frameAllocator.allocate(skyboxLayout);

        // 6. Das Set mit dem Buffer verknüpfen (Write Descriptor Set)
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = *uniformBuffer->getBuffer(),
            .offset = 0,
            .range = uboSize
        };

        vk::WriteDescriptorSet descriptorWrite{
            .dstSet = *cameraSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo
        };

        device.getDevice().updateDescriptorSets(descriptorWrite, nullptr);
	}
};