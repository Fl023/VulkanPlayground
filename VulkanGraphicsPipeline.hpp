#pragma once

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include <vector>
#include <string>

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline(const VulkanDevice& device, const VulkanSwapChain& swapChain);
    ~VulkanGraphicsPipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& getGraphicsPipeline() const;
    [[nodiscard]] const vk::raii::PipelineLayout& getPipelineLayout() const;

private:
    void createGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    static std::vector<char> readFile(const std::string& filename);

private:
    const VulkanDevice& device;
    const VulkanSwapChain& swapChain;
    
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline       graphicsPipeline = nullptr;
};