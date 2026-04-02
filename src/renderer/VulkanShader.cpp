#include "VulkanShader.hpp"
#include "spirv_reflect.h"

// ==========================================
// BASE CLASS IMPLEMENTATION
// ==========================================

VulkanShader::VulkanShader(const VulkanDevice& device, const std::string& filepath, const PipelineSignature* borrowedSignature)
    : m_device(device)
{
    auto shaderCode = readFile(filepath);
    
    vk::ShaderModuleCreateInfo createInfo{ 
        .codeSize = shaderCode.size(), 
        .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data()) 
    };
    
    m_shaderModule = vk::raii::ShaderModule(m_device.getDevice(), createInfo);

    if (borrowedSignature != nullptr) {
        // The user provided a signature (e.g., the Global Opaque Signature)
        m_ActiveSignature = borrowedSignature;
    }
    else {
        // The user provided NOTHING. We must generate and own our signature!
        m_OwnedSignature = std::make_unique<PipelineSignature>();
        reflect(shaderCode); // reflect() will now populate m_OwnedSignature!

        // Point the active signature to our newly built one
        m_ActiveSignature = m_OwnedSignature.get();
    }
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

void VulkanShader::reflect(const std::vector<char>& shaderCode)
{
    SpvReflectShaderModule reflectModule;
    SpvReflectResult result = spvReflectCreateShaderModule(shaderCode.size(), shaderCode.data(), &reflectModule);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to reflect shader!");
    }

    // --- EXTRACT DESCRIPTOR SETS ---
    uint32_t count = 0;
    spvReflectEnumerateDescriptorSets(&reflectModule, &count, NULL);
    std::vector<SpvReflectDescriptorSet*> sets(count);
    spvReflectEnumerateDescriptorSets(&reflectModule, &count, sets.data());

    for (uint32_t i = 0; i < sets.size(); i++) {
        const SpvReflectDescriptorSet& reflectSet = *(sets[i]);

        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        std::vector<vk::DescriptorBindingFlags> bindingFlags; // We need a parallel array for the flags!
        bool isBindlessSet = false;

        for (uint32_t j = 0; j < reflectSet.binding_count; j++) {
            const SpvReflectDescriptorBinding& reflectBinding = *(reflectSet.bindings[j]);

            // Map the SPIR-V type to the Vulkan type
            vk::DescriptorType descType = static_cast<vk::DescriptorType>(reflectBinding.descriptor_type);

            vk::DescriptorSetLayoutBinding binding{
                .binding = reflectBinding.binding,
                .descriptorType = descType,
                .descriptorCount = reflectBinding.count, // Handles arrays!
                .stageFlags = vk::ShaderStageFlagBits::eAllGraphics
            };
            bindings.push_back(binding);

            // --- BINDLESS DETECTION OVERRIDE ---
            // If the shader defines a massive array (e.g., textures[1000]) or an unbounded array (count == 0)
            if (reflectBinding.count >= 1000 || reflectBinding.count == 0) {
                isBindlessSet = true;

                // Apply your exact bindless flags!
                bindingFlags.push_back(
                    vk::DescriptorBindingFlagBits::ePartiallyBound |
                    vk::DescriptorBindingFlagBits::eUpdateAfterBind |
                    vk::DescriptorBindingFlagBits::eVariableDescriptorCount
                );

                // If the shader used an unbounded array like `textures[]`, explicitly set it to your engine's MAX_TEXTURES
                if (reflectBinding.count == 0) {
                    bindings.back().descriptorCount = 1000;
                }
            }
            else {
                // Normal bindings (like your UBO or Skybox cubemap) get no special flags
                bindingFlags.push_back(vk::DescriptorBindingFlags{});
            }
        }

        // Create the Vulkan Layout automatically!
        vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        // If we detected a bindless array in this set, we must attach the pNext chain!
        vk::DescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
        if (isBindlessSet) {
            extendedInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
            extendedInfo.pBindingFlags = bindingFlags.data();

            layoutInfo.pNext = &extendedInfo;
            // Add the pool flag required for bindless sets
            layoutInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
        }

        m_OwnedSignature->AddSetLayout(reflectSet.set, { m_device.getDevice(), layoutInfo });
    }

    // --- EXTRACT PUSH CONSTANTS ---
    uint32_t pushConstantCount = 0;
    spvReflectEnumeratePushConstantBlocks(&reflectModule, &pushConstantCount, NULL);
    std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
    spvReflectEnumeratePushConstantBlocks(&reflectModule, &pushConstantCount, pushConstants.data());

    for (uint32_t i = 0; i < pushConstantCount; i++) {
        const SpvReflectBlockVariable& reflectPushConstant = *(pushConstants[i]);

        m_OwnedSignature->AddPushConstantRange(
            reflectPushConstant.size,
            vk::ShaderStageFlagBits::eAllGraphics,
            reflectPushConstant.offset
		);
    }

    m_OwnedSignature->Build(m_device.getDevice());

    // Cleanup
    spvReflectDestroyShaderModule(&reflectModule);
}