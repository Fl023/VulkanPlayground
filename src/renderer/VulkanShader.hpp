#pragma once

#include "VulkanDevice.hpp"
#include <string>
#include <vector>

class VulkanShader {
public:
    VulkanShader(const VulkanDevice& device, const std::string& filepath);
    virtual ~VulkanShader() = default;

    const vk::raii::ShaderModule& getModule() const { return m_shaderModule; }
    
    const std::vector<vk::raii::DescriptorSetLayout>& getLayouts() const { return m_layouts; }

protected:
    std::vector<char> readFile(const std::string& filename) const;

    // Pure virtual function: Forces every specific shader to define its own bindings
    virtual void createDescriptorSetLayouts() = 0;

protected:
    const VulkanDevice& m_device;
    vk::raii::ShaderModule m_shaderModule = nullptr;
    
    std::vector<vk::raii::DescriptorSetLayout> m_layouts;


};

// ==========================================
// CONCRETE IMPLEMENTATION: Main Shader
// ==========================================
class MainGraphicsShader : public VulkanShader {
public:
    MainGraphicsShader(const VulkanDevice& device, const std::string& filepath);

protected:
    void createDescriptorSetLayouts() override;
};

// ==========================================
// Skybox Shader
// ==========================================
class SkyboxShader : public VulkanShader {
public:
    SkyboxShader(const VulkanDevice& device, const std::string& filepath);

protected:
    void createDescriptorSetLayouts() override;
};