#include "VulkanBuffer.hpp"


VulkanBuffer::VulkanBuffer(const VulkanDevice& device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	: device(device), bufferSize(size) {

	vk::BufferCreateInfo bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
	buffer = vk::raii::Buffer(device.getDevice(), bufferInfo);

	vk::MemoryRequirements memReqs = buffer.getMemoryRequirements();
	vk::MemoryAllocateInfo allocInfo{
		.allocationSize = memReqs.size,
		.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, properties)
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

