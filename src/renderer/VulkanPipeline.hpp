#pragma once

#include "VulkanDevice.hpp"
#include "VulkanShader.hpp"

struct PipelineConfigInfo {
    vk::PipelineViewportStateCreateInfo viewportInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
    vk::PipelineMultisampleStateCreateInfo multisampleInfo;
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<vk::DynamicState> dynamicStateEnables;
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;

    std::vector<vk::Format> colorAttachmentFormats;
    vk::Format depthAttachmentFormat = vk::Format::eUndefined;

    std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
};

class VulkanPipeline {
public:
    VulkanPipeline(const VulkanDevice& device, const VulkanShader& shader, const PipelineConfigInfo& config);
    ~VulkanPipeline() = default;

    // Disallow copying
    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    const vk::raii::Pipeline& getPipeline() const { return m_pipeline; }
    const vk::raii::PipelineLayout& getPipelineLayout() const { return m_pipelineSignature->GetLayout(); }

    // Helper to fill the struct with your standard 3D mesh settings
    static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo, vk::SampleCountFlagBits msaaSamples);

private:
    const VulkanDevice& m_device;
    const PipelineSignature* m_pipelineSignature = nullptr;
    vk::raii::Pipeline m_pipeline = nullptr;
};