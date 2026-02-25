#include "VulkanDescriptorAllocator.hpp"


DescriptorAllocator::DescriptorAllocator(const vk::raii::Device& device, uint32_t initialSets, std::vector<vk::DescriptorPoolSize> poolSizes)
    : m_device(device), m_setsPerPool(initialSets), m_poolSizes(std::move(poolSizes)) 
{
    m_readyPools.push_back(create_pool());
}

void DescriptorAllocator::reset_pools() {
    for (auto& pool : m_readyPools) {
        pool.reset();
    }
    for (auto& pool : m_fullPools) {
        pool.reset();
        m_readyPools.push_back(std::move(pool));
    }
    m_fullPools.clear();
}

vk::raii::DescriptorSet DescriptorAllocator::allocate(const vk::raii::DescriptorSetLayout& layout) {
    vk::raii::DescriptorPool* poolToUse = get_pool();

    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = *poolToUse,
        .descriptorSetCount = 1,
        .pSetLayouts = &*layout
    };

    try {
        return std::move(m_device.allocateDescriptorSets(allocInfo).front()); 

    } catch (const vk::OutOfPoolMemoryError&) {
        m_fullPools.push_back(std::move(*poolToUse));
        m_readyPools.pop_back();

        poolToUse = get_pool();
        allocInfo.setDescriptorPool(**poolToUse);

        return std::move(m_device.allocateDescriptorSets(allocInfo).front());
    } catch (const vk::FragmentedPoolError&) {

        m_fullPools.push_back(std::move(*poolToUse));
        m_readyPools.pop_back();
        poolToUse = get_pool();
        allocInfo.setDescriptorPool(**poolToUse);
        return std::move(m_device.allocateDescriptorSets(allocInfo).front());
    }
}

vk::raii::DescriptorPool* DescriptorAllocator::get_pool() {
    if (!m_readyPools.empty()) {
        return &m_readyPools.back();
    }
    m_readyPools.push_back(create_pool());
    return &m_readyPools.back();
}

vk::raii::DescriptorPool DescriptorAllocator::create_pool() {
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = m_setsPerPool,
        .poolSizeCount = static_cast<uint32_t>(m_poolSizes.size()),
        .pPoolSizes = m_poolSizes.data()
    };
    return vk::raii::DescriptorPool(m_device, poolInfo);
}
