#include "VulkanGraphicsPipeline.hpp"
#include "VulkanVertex.hpp"


VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanDevice& device, const VulkanSwapChain& swapChain)
    : device(device), swapChain(swapChain)
{
    createDescriptorSetLayout();
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

void VulkanGraphicsPipeline::createDescriptorSetLayout()
{
    // 1. Layout für Set 0 (Kamera)
    vk::DescriptorSetLayoutBinding globalBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
    vk::DescriptorSetLayoutCreateInfo globalLayoutInfo{ .bindingCount = 1, .pBindings = &globalBinding };
    globalSetLayout = vk::raii::DescriptorSetLayout(device.getDevice(), globalLayoutInfo);

    // 2. Layout für Set 1 (Material)
    constexpr uint32_t MAX_TEXTURES = 1000;
    vk::DescriptorSetLayoutBinding materialBinding(0, vk::DescriptorType::eCombinedImageSampler, MAX_TEXTURES, vk::ShaderStageFlagBits::eFragment, nullptr);

    vk::DescriptorBindingFlags bindlessFlags =
        vk::DescriptorBindingFlagBits::ePartiallyBound |
        vk::DescriptorBindingFlagBits::eUpdateAfterBind |
        vk::DescriptorBindingFlagBits::eVariableDescriptorCount;

    vk::DescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{
        .bindingCount = 1,
        .pBindingFlags = &bindlessFlags
    };

    vk::DescriptorSetLayoutCreateInfo materialLayoutInfo{
        .pNext = &extendedInfo, // Chain the flags
        .flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, // Important!
        .bindingCount = 1,
        .pBindings = &materialBinding
    };    
    
    materialSetLayout = vk::raii::DescriptorSetLayout(device.getDevice(), materialLayoutInfo);
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


    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{ .vertexBindingDescriptionCount = 1,
                                                            .pVertexBindingDescriptions = &bindingDescription,
                                                            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
                                                            .pVertexAttributeDescriptions = attributeDescriptions.data() };
    
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
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False, 
        .depthBiasSlopeFactor = 1.0f, 
        .lineWidth = 1.0f 
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{ 
        .rasterizationSamples = device.getMsaaSamples(),
        .sampleShadingEnable = vk::True,
        .minSampleShading = 1.0f
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
            .depthTestEnable = vk::True,
            .depthWriteEnable = vk::True,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable = vk::False };

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

    vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(PushConstants) // 64 Bytes für deine Model-Matrix
    };

    std::array<vk::DescriptorSetLayout, 2> layouts = { *globalSetLayout, *materialSetLayout };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    pipelineLayout = vk::raii::PipelineLayout(device.getDevice(), pipelineLayoutInfo);

    vk::Format colorAttachmentFormat = swapChain.getSurfaceFormat().format;

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorAttachmentFormat,
        .depthAttachmentFormat = vk::Format::eD32Sfloat
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
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