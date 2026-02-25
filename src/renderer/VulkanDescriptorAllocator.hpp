#include "renderer/VulkanDevice.hpp"

class DescriptorAllocator {
public:
    DescriptorAllocator(const vk::raii::Device& device, uint32_t initialSets, std::vector<vk::DescriptorPoolSize> poolSizes);

    void reset_pools();
    vk::raii::DescriptorSet allocate(const vk::raii::DescriptorSetLayout& layout);

private:
    vk::raii::DescriptorPool* get_pool();
    vk::raii::DescriptorPool create_pool();

private:
    const vk::raii::Device& m_device;
    uint32_t m_setsPerPool;
    std::vector<vk::DescriptorPoolSize> m_poolSizes;
    
    std::vector<vk::raii::DescriptorPool> m_fullPools;
    std::vector<vk::raii::DescriptorPool> m_readyPools;


};