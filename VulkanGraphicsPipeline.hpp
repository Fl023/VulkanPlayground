#pragma once

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline(const VulkanDevice& device, const VulkanSwapChain& swapChain);
    ~VulkanGraphicsPipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& getGraphicsPipeline() const;
    [[nodiscard]] const vk::raii::PipelineLayout& getPipelineLayout() const;
	[[nodiscard]] const vk::raii::DescriptorSetLayout& getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
	void createDescriptorSetLayout();
    void createGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    static std::vector<char> readFile(const std::string& filename);

private:
    const VulkanDevice& device;
    const VulkanSwapChain& swapChain;
    
    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline       graphicsPipeline = nullptr;
};