#pragma once

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

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
	vk::raii::DescriptorPool descriptorPool = nullptr;
	vk::raii::DescriptorSet descriptorSet = nullptr;

	VulkanFrame(const VulkanDevice& device, const vk::raii::DescriptorSetLayout& layout, vk::DeviceSize uboSize)
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

        // 4. Descriptor Pool für dieses Frame erstellen
        // Wir brauchen Platz für genau ein Uniform Buffer Set
        std::array poolSize{
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
        };

        vk::DescriptorPoolCreateInfo descriptorPoolInfo{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1,
            .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
            .pPoolSizes = poolSize.data()
        };
        descriptorPool = vk::raii::DescriptorPool(device.getDevice(), descriptorPoolInfo);

        // 5. Descriptor Set allokieren
        vk::DescriptorSetAllocateInfo descriptorSetAllocInfo{
            .descriptorPool = *descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*layout
        };
        descriptorSet = std::move(device.getDevice().allocateDescriptorSets(descriptorSetAllocInfo).front());

        // 6. Das Set mit dem Buffer verknüpfen (Write Descriptor Set)
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = *uniformBuffer->getBuffer(),
            .offset = 0,
            .range = uboSize
        };

        vk::WriteDescriptorSet descriptorWrite{
            .dstSet = *descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo
        };

        device.getDevice().updateDescriptorSets(descriptorWrite, nullptr);
	}
};