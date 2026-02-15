#pragma once

#include "VulkanDevice.hpp"

struct VulkanFrame {
	// Command Resources
	vk::raii::CommandPool commandPool = nullptr;
	vk::raii::CommandBuffer commandBuffer = nullptr;

	// Synchronization Primitives
	vk::raii::Semaphore presentCompleteSemaphore = nullptr; // GPU-GPU: Swapchain -> Render
	vk::raii::Semaphore renderFinishedSemaphore = nullptr; // GPU-GPU: Render -> Present
	vk::raii::Fence inFlightFence = nullptr;               // GPU-CPU: GPU done -> CPU can reuse frame

	VulkanFrame(const VulkanDevice& device)
	{
		vk::CommandPoolCreateInfo poolInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = device.getQueueIndex() };
		commandPool = vk::raii::CommandPool(device.getDevice(), poolInfo);

		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
		commandBuffer = std::move(vk::raii::CommandBuffers(device.getDevice(), allocInfo).front());

		presentCompleteSemaphore = vk::raii::Semaphore(device.getDevice(), vk::SemaphoreCreateInfo());
		renderFinishedSemaphore = vk::raii::Semaphore(device.getDevice(), vk::SemaphoreCreateInfo());
		inFlightFence = vk::raii::Fence(device.getDevice(), { .flags = vk::FenceCreateFlagBits::eSignaled });
	}
};