#pragma once

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

struct PushConstants {
    glm::mat4 modelMatrix;
    uint32_t textureIndex;
};

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline(const VulkanDevice& device, const VulkanSwapChain& swapChain);
    ~VulkanGraphicsPipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& getGraphicsPipeline() const;
    [[nodiscard]] const vk::raii::PipelineLayout& getPipelineLayout() const;
	[[nodiscard]] const vk::raii::DescriptorSetLayout& getGlobalDescriptorSetLayout() const { return globalSetLayout; }
    [[nodiscard]] const vk::raii::DescriptorSetLayout& getMaterialDescriptorSetLayout() const { return materialSetLayout; }


private:
	void createDescriptorSetLayout();
    void createGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    static std::vector<char> readFile(const std::string& filename);

private:
    const VulkanDevice& device;
    const VulkanSwapChain& swapChain;
    
    vk::raii::DescriptorSetLayout globalSetLayout = nullptr;
    vk::raii::DescriptorSetLayout materialSetLayout = nullptr;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline       graphicsPipeline = nullptr;
};