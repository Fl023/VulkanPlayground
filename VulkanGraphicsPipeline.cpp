#include "VulkanGraphicsPipeline.hpp"


VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanDevice& device, const VulkanSwapChain& swapChain)
    : device(device), swapChain(swapChain)
{
    createGraphicsPipeline();
}

const vk::raii::Pipeline& VulkanGraphicsPipeline::getGraphicsPipeline() const
{
    return graphicsPipeline;
}

const vk::raii::PipelineLayout& VulkanGraphicsPipeline::getPipelineLayout() const
{
    return pipelineLayout;
}

std::vector<char> VulkanGraphicsPipeline::readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

void VulkanGraphicsPipeline::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("shaders/slang.spv");

    vk::raii::ShaderModule shaderModule = createShaderModule(vertShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain" 
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain" 
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ 
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False
    };

    vk::PipelineViewportStateCreateInfo viewportState{ 
        .viewportCount = 1, 
        .scissorCount = 1 
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{ 
        .depthClampEnable = vk::False, 
        .rasterizerDiscardEnable = vk::False, 
        .polygonMode = vk::PolygonMode::eFill, 
        .cullMode = vk::CullModeFlagBits::eBack, 
        .frontFace = vk::FrontFace::eClockwise, 
        .depthBiasEnable = vk::False, 
        .depthBiasSlopeFactor = 1.0f, 
        .lineWidth = 1.0f 
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{ 
        .rasterizationSamples = vk::SampleCountFlagBits::e1, 
        .sampleShadingEnable = vk::False 
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{ 
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA 
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{ 
        .logicOpEnable = vk::False, 
        .logicOp = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments = &colorBlendAttachment 
    };

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor 
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{ 
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), 
        .pDynamicStates = dynamicStates.data() 
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
        .setLayoutCount = 0, 
        .pushConstantRangeCount = 0 
    };

    pipelineLayout = vk::raii::PipelineLayout(device.getDevice(), pipelineLayoutInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChain.getSurfaceFormat().format
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = nullptr 
    };

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain(
        pipelineInfo,
        pipelineRenderingCreateInfo
    );

    graphicsPipeline = vk::raii::Pipeline(
        device.getDevice(), 
        nullptr, 
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()
    );
}

vk::raii::ShaderModule VulkanGraphicsPipeline::createShaderModule(const std::vector<char>& code) const
{
    vk::ShaderModuleCreateInfo createInfo{ 
        .codeSize = code.size(), 
        .pCode = reinterpret_cast<const uint32_t*>(code.data()) 
    };
    
    return vk::raii::ShaderModule(device.getDevice(), createInfo);
}