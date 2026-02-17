#pragma once

#include "VulkanDevice.hpp"

class VulkanBuffer {
public:
    VulkanBuffer(const VulkanDevice& device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    ~VulkanBuffer() = default;

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&&) noexcept = default;
    VulkanBuffer& operator=(VulkanBuffer&&) noexcept = default;

    const vk::raii::Buffer& getBuffer() const { return buffer; }
    vk::DeviceSize getSize() const { return bufferSize; }

    void upload(const void* data, vk::DeviceSize size);

    static void copyBuffer(const VulkanDevice& device, const vk::raii::Buffer& src, const vk::raii::Buffer& dst, vk::DeviceSize size);

private:
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    const VulkanDevice& device;
    vk::DeviceSize bufferSize;
    vk::raii::Buffer buffer = nullptr;
    vk::raii::DeviceMemory bufferMemory = nullptr;
};