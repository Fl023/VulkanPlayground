#pragma once

#include "renderer/VulkanDevice.hpp"
#include "renderer/VulkanBuffer.hpp"
#include "renderer/VulkanVertex.hpp"

class Mesh {
public:
    Mesh(const VulkanDevice& device, const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    const std::string& GetName() const { return m_Name; }
    const vk::raii::Buffer& getVertexBuffer() const { return m_VertexBuffer->getBuffer(); }
    const vk::raii::Buffer& getIndexBuffer() const { return m_IndexBuffer->getBuffer(); }
    uint32_t getIndexCount() const { return m_IndexCount; }

private:
    std::string m_Name;
    std::optional<VulkanBuffer> m_VertexBuffer;
    std::optional<VulkanBuffer> m_IndexBuffer;
    uint32_t m_VertexCount;
    uint32_t m_IndexCount;
};