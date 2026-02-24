#include "VulkanDevice.hpp"


VulkanDevice::VulkanDevice(const VulkanContext& context, const VulkanWindow& window)
    : context(context), window(window) 
{
    surface = window.createSurface(context.getInstance());
    pickPhysicalDevice();
    createLogicalDevice();
    createDefaultSampler();
}

vk::raii::CommandBuffer VulkanDevice::beginSingleTimeCommands() const
{
    vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
    vk::raii::CommandBuffer commandBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void VulkanDevice::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer) const
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
}

void VulkanDevice::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) const
{
    vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
    commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    endSingleTimeCommands(commandCopyBuffer);
}

void VulkanDevice::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) const
{
    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();
    vk::BufferImageCopy region{ .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0, .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1}, .imageOffset = {0, 0, 0}, .imageExtent = {width, height, 1} };

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });

    endSingleTimeCommands(commandBuffer);
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanDevice::transitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::ImageAspectFlags imageAspectFlags) const
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = imageAspectFlags,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    commandBuffer.pipelineBarrier2(dependencyInfo);
}

const vk::raii::Device& VulkanDevice::getDevice() const { return device; }
const vk::raii::PhysicalDevice& VulkanDevice::getPhysicalDevice() const { return physicalDevice; }
const vk::raii::SurfaceKHR& VulkanDevice::getSurface() const { return surface; }
const vk::raii::Queue& VulkanDevice::getQueue() const { return queue; }
const uint32_t& VulkanDevice::getQueueIndex() const { return queueIndex; }

void VulkanDevice::pickPhysicalDevice() {
    auto devices = context.getInstance().enumeratePhysicalDevices();
    
    for (const auto& d : devices) {
        if (isDeviceSuitable(d)) {
            physicalDevice = d;
            msaaSamples = getMaxUsableSampleCount();
            vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
            std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
            std::cout << "API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
                      << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
                      << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
            return;
        }
    }
    
    throw std::runtime_error("failed to find a suitable GPU supporting the profile!");
}

bool VulkanDevice::isDeviceSuitable(const vk::raii::PhysicalDevice& deviceToCheck) const {
    VkBool32 supported = VK_FALSE;
    vpGetPhysicalDeviceProfileSupport(*context.getInstance(), *deviceToCheck, &profile, &supported);

    auto qProps = deviceToCheck.getQueueFamilyProperties();
    bool hasGraphics = std::ranges::any_of(qProps, [](auto q) { 
        return !!(q.queueFlags & vk::QueueFlagBits::eGraphics); 
    });

    return supported && hasGraphics;
}

void VulkanDevice::createLogicalDevice() {
    auto qProps = physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < qProps.size(); ++i) {
        if ((qProps[i].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(i, *surface)) {
            queueIndex = i;
            break;
        }
    }

    if (queueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    float priority = 1.0f;
    vk::DeviceQueueCreateInfo qInfo{ 
        .queueFamilyIndex = queueIndex, 
        .queueCount = 1, 
        .pQueuePriorities = &priority 
    };

    vk::DeviceCreateInfo deviceCreateInfo{
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()
    };

    VpDeviceCreateInfo vpCreateInfo{
        .pCreateInfo = reinterpret_cast<const VkDeviceCreateInfo*>(&deviceCreateInfo),
        .enabledFullProfileCount = 1,
        .pEnabledFullProfiles = &profile
    };

    VkDevice rawDevice;
    if (vpCreateDevice(*physicalDevice, &vpCreateInfo, nullptr, &rawDevice) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Profile-compliant logical device!");
    }

    device = vk::raii::Device(physicalDevice, rawDevice);
    
    queue = vk::raii::Queue(device, queueIndex, 0);

    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = queueIndex
    };
    commandPool = vk::raii::CommandPool(device, poolInfo);
}

void VulkanDevice::createDefaultSampler()
{
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo        samplerInfo{
               .magFilter = vk::Filter::eLinear,
               .minFilter = vk::Filter::eLinear,
               .mipmapMode = vk::SamplerMipmapMode::eLinear,
               .addressModeU = vk::SamplerAddressMode::eRepeat,
               .addressModeV = vk::SamplerAddressMode::eRepeat,
               .addressModeW = vk::SamplerAddressMode::eRepeat,
               .mipLodBias = 0.0f,
               .anisotropyEnable = vk::True,
               .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
               .compareEnable = vk::False,
               .compareOp = vk::CompareOp::eAlways,
               .minLod = 0.0f,
               .maxLod = vk::LodClampNone,
               .borderColor = vk::BorderColor::eIntOpaqueBlack,
               .unnormalizedCoordinates = vk::False };
    defaultSampler = vk::raii::Sampler(device, samplerInfo);
}

vk::SampleCountFlagBits VulkanDevice::getMaxUsableSampleCount()
{
    vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();

    vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

    return vk::SampleCountFlagBits::e1;
}
