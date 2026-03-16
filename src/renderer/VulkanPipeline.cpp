#include "VulkanPipeline.hpp"
#include "VulkanVertex.hpp" // Ensure you include your vertex definition

void VulkanPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo, vk::SampleCountFlagBits msaaSamples) {
    // Ported from your original VulkanGraphicsPipeline
    configInfo.inputAssemblyInfo = { .topology = vk::PrimitiveTopology::eTriangleList, .primitiveRestartEnable = vk::False };
    
    configInfo.viewportInfo = { .viewportCount = 1, .scissorCount = 1 }; 
    
    configInfo.rasterizationInfo = { .depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False, .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack, .frontFace = vk::FrontFace::eCounterClockwise, .depthBiasEnable = vk::False, .lineWidth = 1.0f };
    
    configInfo.multisampleInfo = { .rasterizationSamples = msaaSamples, .sampleShadingEnable = vk::True, .minSampleShading = 1.0f };
    
    configInfo.colorBlendAttachment = { .blendEnable = vk::False, .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
    
    configInfo.colorBlendInfo = { .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &configInfo.colorBlendAttachment };
    
    configInfo.depthStencilInfo = { .depthTestEnable = vk::True, .depthWriteEnable = vk::True, .depthCompareOp = vk::CompareOp::eLess, .depthBoundsTestEnable = vk::False, .stencilTestEnable = vk::False };
    
    configInfo.dynamicStateEnables = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    
    configInfo.dynamicStateInfo = { .dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size()), .pDynamicStates = configInfo.dynamicStateEnables.data() };

    configInfo.bindingDescriptions = { Vertex::getBindingDescription() };
    auto attributes = Vertex::getAttributeDescriptions();
    configInfo.attributeDescriptions = std::vector<vk::VertexInputAttributeDescription>(attributes.begin(), attributes.end());
}

VulkanPipeline::VulkanPipeline(const VulkanDevice& device, const VulkanShader& shader, const PipelineConfigInfo& config)
    : m_device(device)
{
    // 1. Get the RAII layouts via const ref
    const auto& raiiLayouts = shader.getLayouts();
    
    // 2. Extract the raw handles locally into a temporary vector
    std::vector<vk::DescriptorSetLayout> rawLayouts;
    rawLayouts.reserve(raiiLayouts.size());
    for (const auto& layout : raiiLayouts) {
        rawLayouts.push_back(*layout); // Extracts the raw vk::DescriptorSetLayout
    }

    // 3. Create Pipeline Layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(rawLayouts.size()),
        .pSetLayouts = rawLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size()),
        .pPushConstantRanges = config.pushConstantRanges.data()
    };
    m_pipelineLayout = vk::raii::PipelineLayout(m_device.getDevice(), pipelineLayoutInfo);

    // 4. Setup Shader Stages
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        { .stage = vk::ShaderStageFlagBits::eVertex, .module = shader.getModule(), .pName = "vertMain" },
        { .stage = vk::ShaderStageFlagBits::eFragment, .module = shader.getModule(), .pName = "fragMain" }
    };

    // 5. Vertex Input
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = static_cast<uint32_t>(config.bindingDescriptions.size()),
        .pVertexBindingDescriptions = config.bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(config.attributeDescriptions.size()),
        .pVertexAttributeDescriptions = config.attributeDescriptions.data()
    };

    // 6. Setup Dynamic Rendering
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = static_cast<uint32_t>(config.colorAttachmentFormats.size()),
        .pColorAttachmentFormats = config.colorAttachmentFormats.data(),
        .depthAttachmentFormat = config.depthAttachmentFormat
    };

    // 7. Create the Graphics Pipeline
    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .stageCount = 2, .pStages = shaderStages, .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &config.inputAssemblyInfo, .pViewportState = &config.viewportInfo,
        .pRasterizationState = &config.rasterizationInfo, .pMultisampleState = &config.multisampleInfo,
        .pDepthStencilState = &config.depthStencilInfo, .pColorBlendState = &config.colorBlendInfo,
        .pDynamicState = &config.dynamicStateInfo, .layout = *m_pipelineLayout, .renderPass = nullptr 
    };

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> chain(pipelineInfo, pipelineRenderingCreateInfo);
    m_pipeline = vk::raii::Pipeline(m_device.getDevice(), nullptr, chain.get<vk::GraphicsPipelineCreateInfo>());
}