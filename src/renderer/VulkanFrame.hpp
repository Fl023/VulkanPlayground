#pragma once

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDescriptorAllocator.hpp"

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
    std::unique_ptr<DescriptorSetInstance> globalSet;

	VulkanFrame(const VulkanDevice& device, 
        const vk::raii::DescriptorSetLayout& globalLayout, 
        vk::DeviceSize uboSize,
        DescriptorAllocator* globalAllocator)
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

        globalSet = std::make_unique<DescriptorSetInstance>(device.getDevice(), globalLayout, globalAllocator);

        globalSet->WriteBuffer(
            0,
            *uniformBuffer->getBuffer(),
            uboSize,
            vk::DescriptorType::eUniformBuffer
        );
	}
};