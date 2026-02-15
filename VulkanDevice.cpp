#include "VulkanDevice.hpp"


VulkanDevice::VulkanDevice(const VulkanContext& context, const VulkanWindow& window)
    : context(context), window(window) 
{
    surface = window.createSurface(context.getInstance());
    pickPhysicalDevice();
    createLogicalDevice();
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
}