#include "RGCommandList.hpp"
#include "VulkanPipeline.hpp"
#include "scene/Mesh.hpp"

void RGCommandList::SetGlobalState(vk::DescriptorSet globalUbo, vk::DescriptorSet bindlessArray)
{
    m_GlobalUbo = globalUbo;
    m_BindlessArray = bindlessArray;
}

void RGCommandList::BindPipeline(VulkanPipeline* pipeline) {
    m_Cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->getPipeline());

    std::array<vk::DescriptorSet, 2> sets = { m_GlobalUbo, m_BindlessArray };
    m_Cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipeline->getPipelineLayout(),
        0, // Start at Set 0
        sets,
        nullptr
    );
}

void RGCommandList::PushConstants(vk::PipelineLayout layout, vk::ShaderStageFlags stages, uint32_t offset, uint32_t size, const void* data) {
    m_Cmd.pushConstants(layout, stages, offset, size, data);
}

void RGCommandList::DrawMesh(Mesh* mesh) {
    m_Cmd.bindVertexBuffers(0, *mesh->getVertexBuffer(), { 0 });
    m_Cmd.bindIndexBuffer(*mesh->getIndexBuffer(), 0, vk::IndexType::eUint32);
    m_Cmd.drawIndexed(mesh->getIndexCount(), 1, 0, 0, 0);
}