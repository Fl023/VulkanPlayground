#pragma once

#include "scene/AssetHandle.hpp"
#include "VulkanPipeline.hpp"

// Forward declarations
class VulkanPipeline;
class Mesh;

class RGCommandList {
public:
    // Internal backend constructor
    RGCommandList(vk::CommandBuffer cmd) : m_Cmd(cmd) {}

    void SetGlobalState(vk::DescriptorSet globalUbo, vk::DescriptorSet bindlessArray);
    void BindPipeline(VulkanPipeline* pipeline);
    
    void PushConstants(vk::PipelineLayout layout, vk::ShaderStageFlags stages, uint32_t offset, uint32_t size, const void* data);
    
    template<typename T>
    void PushConstants(vk::PipelineLayout layout, vk::ShaderStageFlags stages, uint32_t offset, const T& data) {
        PushConstants(layout, stages, offset, sizeof(T), &data);
    }
    
    void DrawMesh(Mesh* mesh);

    // Escape hatch if you absolutely need the raw buffer (e.g., for ImGui)
    vk::CommandBuffer GetRaw() const { return m_Cmd; }

private:
    vk::CommandBuffer m_Cmd;
    vk::DescriptorSet m_GlobalUbo;
    vk::DescriptorSet m_BindlessArray;
};