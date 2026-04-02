#pragma once
#include "renderer/VulkanDevice.hpp"

class DescriptorAllocator {
public:
    DescriptorAllocator(const vk::raii::Device& device, 
		uint32_t initialSets, // How many sets should be in each pool before we create a new one?
		std::vector<vk::DescriptorPoolSize> poolSizes, // The sizes (how many descriptors) of each pool (e.g. how many uniform buffers, combined image samplers, etc.)
        vk::DescriptorPoolCreateFlags flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    void reset_pools();
    vk::raii::DescriptorSet allocate(const vk::raii::DescriptorSetLayout& layout);

private:
    vk::raii::DescriptorPool* get_pool();
    vk::raii::DescriptorPool create_pool();

private:
    const vk::raii::Device& m_device;
    uint32_t m_setsPerPool;
    std::vector<vk::DescriptorPoolSize> m_poolSizes;
    vk::DescriptorPoolCreateFlags m_poolFlags;
    
    std::vector<vk::raii::DescriptorPool> m_fullPools;
    std::vector<vk::raii::DescriptorPool> m_readyPools;
};


class PipelineSignature {
public:
    PipelineSignature() {};

    void AddSetLayout(uint32_t setIndex, vk::raii::DescriptorSetLayout layout) {
        m_SetLayouts.insert_or_assign(setIndex, std::move(layout));
    }

    void AddPushConstantRange(uint32_t size, vk::ShaderStageFlags stages, uint32_t offset = 0) {
        m_PushConstantRanges.push_back({ stages, offset, size });
    }

    void Build(const vk::raii::Device& device) {
        // Extract the raw layouts to pass to Vulkan
        std::vector<vk::DescriptorSetLayout> rawLayouts;
        for (const auto& [index, layout] : m_SetLayouts) {
            rawLayouts.push_back(*layout);
        }

        vk::PipelineLayoutCreateInfo info{
            .setLayoutCount = static_cast<uint32_t>(rawLayouts.size()),
            .pSetLayouts = rawLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(m_PushConstantRanges.size()),
            .pPushConstantRanges = m_PushConstantRanges.data()
        };
        m_PipelineLayout = vk::raii::PipelineLayout(device, info);
    }

    const vk::raii::PipelineLayout& GetLayout() const { return m_PipelineLayout; }
    const vk::raii::DescriptorSetLayout& GetSetLayout(uint32_t index) const { return m_SetLayouts.at(index); }

private:
    std::map<uint32_t, vk::raii::DescriptorSetLayout> m_SetLayouts;
    std::vector<vk::PushConstantRange> m_PushConstantRanges;
    vk::raii::PipelineLayout m_PipelineLayout = nullptr;
};


class DescriptorSetInstance {
public:
    // We pass the Blueprint (from the Signature) to the Allocator!
    DescriptorSetInstance(const vk::raii::Device& device,
        const vk::raii::DescriptorSetLayout& layout,
        DescriptorAllocator* allocator)
        : m_Device(device)
    {
        m_AllocatedSet = allocator->allocate(layout);
    }

    // A nice intuitive wrapper to update a buffer!
    void WriteBuffer(uint32_t binding, vk::Buffer buffer, size_t size, vk::DescriptorType type) {
        vk::DescriptorBufferInfo bufferInfo{ buffer, 0, size };

        vk::WriteDescriptorSet write{
            .dstSet = *m_AllocatedSet,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = type,
            .pBufferInfo = &bufferInfo
        };
        m_Device.updateDescriptorSets(write, nullptr);
    }

    // A nice intuitive wrapper to update an image!
    void WriteImage(uint32_t binding, vk::DescriptorImageInfo imageInfo, uint32_t dstArrayElement = 0) {
        vk::WriteDescriptorSet write{
            .dstSet = *m_AllocatedSet,
            .dstBinding = binding,
            .dstArrayElement = dstArrayElement,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        };
        m_Device.updateDescriptorSets(write, nullptr);
    }

    const vk::raii::DescriptorSet& GetSet() const { return m_AllocatedSet; }

private:
    const vk::raii::Device& m_Device;
    vk::raii::DescriptorSet m_AllocatedSet = nullptr;
};