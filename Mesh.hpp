#pragma once

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanVertex.hpp"

class Mesh {
public:
    // Der Konstruktor erstellt die tatsächlichen Vulkan-Buffer
    Mesh(const VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

    // Getter für den Renderer
    const vk::raii::Buffer& getVertexBuffer() const { return m_VertexBuffer->getBuffer(); }
    const vk::raii::Buffer& getIndexBuffer() const { return m_IndexBuffer->getBuffer(); }
    uint32_t getIndexCount() const { return m_IndexCount; }

private:
    std::unique_ptr<VulkanBuffer> m_VertexBuffer;
    std::unique_ptr<VulkanBuffer> m_IndexBuffer;
    uint32_t m_VertexCount;
    uint32_t m_IndexCount;
};