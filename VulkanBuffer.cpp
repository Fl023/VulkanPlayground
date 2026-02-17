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
}

void VulkanBuffer::upload(const void* data, vk::DeviceSize size) {
	void* mapped = bufferMemory.mapMemory(0, size);
	std::memcpy(mapped, data, static_cast<size_t>(size));
	bufferMemory.unmapMemory();
}

//
//void VulkanBuffer::createVertexBuffer(const std::vector<Vertex>& vertices)
//{
//	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
//
//	vk::raii::Buffer stagingBuffer = nullptr;
//	vk::raii::DeviceMemory stagingBufferMemory = nullptr;
//	createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
//	copyData(vertices.data(), bufferSize, stagingBufferMemory);
//
//	vk::raii::Buffer vertexBuffer = nullptr;
//	vk::raii::DeviceMemory vertexBufferMemory = nullptr;
//	createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);
//
//	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
//}


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

