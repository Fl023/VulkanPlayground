#include "VulkanShader.hpp"

// ==========================================
// BASE CLASS IMPLEMENTATION
// ==========================================

VulkanShader::VulkanShader(const VulkanDevice& device, const std::string& filepath)
    : m_device(device) 
{
    auto shaderCode = readFile(filepath);
    
    vk::ShaderModuleCreateInfo createInfo{ 
        .codeSize = shaderCode.size(), 
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data()) 
    };
    
    m_shaderModule = vk::raii::ShaderModule(m_device.getDevice(), createInfo);
}

std::vector<char> VulkanShader::readFile(const std::string& filename) const 
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

// ==========================================
// MAIN GRAPHICS SHADER IMPLEMENTATION
// ==========================================

MainGraphicsShader::MainGraphicsShader(const VulkanDevice& device, const std::string& filepath)
    : VulkanShader(device, filepath)
{
    // We call this in the derived constructor so the layouts are ready immediately
    createDescriptorSetLayouts(); 
}

void MainGraphicsShader::createDescriptorSetLayouts() 
{
    // 1. Layout for Set 0 (Camera UBO) - Moved from your old pipeline 
    vk::DescriptorSetLayoutBinding globalBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
    vk::DescriptorSetLayoutCreateInfo globalLayoutInfo{ .bindingCount = 1, .pBindings = &globalBinding };
    m_layouts.push_back(vk::raii::DescriptorSetLayout(m_device.getDevice(), globalLayoutInfo));

    // 2. Layout for Set 1 (Material Bindless Array) - Moved from your old pipeline 
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
        .pNext = &extendedInfo, 
        .flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, 
        .bindingCount = 1,
        .pBindings = &materialBinding
    };    
    
    m_layouts.push_back(vk::raii::DescriptorSetLayout(m_device.getDevice(), materialLayoutInfo));
}


// ==========================================
// SKYBOX SHADER IMPLEMENTATION
// ==========================================

SkyboxShader::SkyboxShader(const VulkanDevice& device, const std::string& filepath)
    : VulkanShader(device, filepath)
{
    // Explicit initialization, as requested!
    createDescriptorSetLayouts();
}

void SkyboxShader::createDescriptorSetLayouts()
{
    // 1. Layout for Set 0 (Camera UBO - Identical to Main Shader)
    vk::DescriptorSetLayoutBinding globalBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
    vk::DescriptorSetLayoutCreateInfo globalLayoutInfo{ .bindingCount = 1, .pBindings = &globalBinding };
    m_layouts.push_back(vk::raii::DescriptorSetLayout(m_device.getDevice(), globalLayoutInfo));

    // 2. Layout for Set 1 (Cubemap Texture)
    // Notice this is NOT a bindless array. It's just a single texture!
    vk::DescriptorSetLayoutBinding cubemapBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr);
    vk::DescriptorSetLayoutCreateInfo cubemapLayoutInfo{ .bindingCount = 1, .pBindings = &cubemapBinding };
    m_layouts.push_back(vk::raii::DescriptorSetLayout(m_device.getDevice(), cubemapLayoutInfo));
}