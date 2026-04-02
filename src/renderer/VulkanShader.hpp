#pragma once

#include "VulkanDevice.hpp"
#include "VulkanDescriptorAllocator.hpp"
#include <string>
#include <vector>

class VulkanShader {
public:
    VulkanShader(const VulkanDevice& device, const std::string& filepath, const PipelineSignature* borrowedSignature = nullptr);
    virtual ~VulkanShader() = default;

    const vk::raii::ShaderModule& getModule() const { return m_shaderModule; }
    
    const PipelineSignature* getSignature() const { return m_ActiveSignature; }

protected:
    std::vector<char> readFile(const std::string& filename) const;
    void reflect(const std::vector<char>& shaderCode);

protected:
    const VulkanDevice& m_device;
    vk::raii::ShaderModule m_shaderModule = nullptr;
    
    std::unique_ptr<PipelineSignature> m_OwnedSignature;

    const PipelineSignature* m_ActiveSignature = nullptr;
};