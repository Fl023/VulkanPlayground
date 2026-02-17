#include "VulkanBuffer.hpp"


VulkanBuffer::VulkanBuffer(const VulkanDevice& device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	: device(device), bufferSize(size) {

	vk::BufferCreateInfo bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
	buffer = vk::raii::Buffer(device.getDevice(), bufferInfo);

	vk::MemoryRequirements memReqs = buffer.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo{
		.allocationSize = memReqs.size,
		.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, properties)
	};

	bufferMemory = vk::raii::DeviceMemory(device.getDevice(), allocInfo);
	buffer.bindMemory(*bufferMemory, 0);

	if (properties & vk::MemoryPropertyFlagBits::eHostVisible) {
		mappedData = bufferMemory.mapMemory(0, size);
	}
}

VulkanBuffer::~VulkanBuffer() {
	if (mappedData) {
		bufferMemory.unmapMemory();
	}
}

void VulkanBuffer::upload(const void* data, vk::DeviceSize size) {
	if (mappedData) {
		std::memcpy(mappedData, data, static_cast<size_t>(size));
	}
	else {
		void* tempMapped = bufferMemory.mapMemory(0, size);
		std::memcpy(tempMapped, data, static_cast<size_t>(size));
		bufferMemory.unmapMemory();
	}
}


void VulkanBuffer::copyBuffer(const VulkanDevice& device, const vk::raii::Buffer& src, const vk::raii::Buffer& dst, vk::DeviceSize size)
{
	vk::CommandBufferAllocateInfo allocInfo{ .commandPool = *device.getCommandPool(), .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
	vk::raii::CommandBuffer commandBuffer = std::move(device.getDevice().allocateCommandBuffers(allocInfo).front());

	commandBuffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
	commandBuffer.copyBuffer(*src, *dst, vk::BufferCopy(0, 0, size));
	commandBuffer.end();

	vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
	device.getQueue().submit(submitInfo, nullptr);
	device.getQueue().waitIdle();
}

uint32_t VulkanBuffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	vk::PhysicalDeviceMemoryProperties memProperties = device.getPhysicalDevice().getMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

