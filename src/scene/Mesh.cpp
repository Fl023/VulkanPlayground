#include "Mesh.hpp"

Mesh::Mesh(const VulkanDevice& device, const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	: m_Name(name)
{
    m_VertexCount = static_cast<uint32_t>(vertices.size());
    m_IndexCount = static_cast<uint32_t>(indices.size());

    vk::DeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    vk::DeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();

    VulkanBuffer vertexStagingBuffer(
        device,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    vertexStagingBuffer.upload(vertices.data(), vertexBufferSize);

    m_VertexBuffer.emplace(
        device,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    device.copyBuffer(*vertexStagingBuffer.getBuffer(), *m_VertexBuffer->getBuffer(), vertexBufferSize);


    VulkanBuffer indexStagingBuffer(
        device,
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    indexStagingBuffer.upload(indices.data(), indexBufferSize);

    m_IndexBuffer.emplace(
        device,
        indexBufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    device.copyBuffer(*indexStagingBuffer.getBuffer(), *m_IndexBuffer->getBuffer(), indexBufferSize);
}
