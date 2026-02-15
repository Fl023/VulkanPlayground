#include "VulkanContext.hpp"


VulkanContext::VulkanContext()
{
    createInstance();
    setupDebugMessenger();
}

VulkanContext::~VulkanContext() = default;

const vk::raii::Instance& VulkanContext::getInstance() const
{
    return instance;
}

void VulkanContext::createInstance()
{
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    auto requiredExtensions = getRequiredExtensions();

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (auto const& requiredExtension : requiredExtensions)
    {
        bool found = std::ranges::any_of(extensionProperties,
            [requiredExtension](auto const& extensionProperty) {
                return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
            });

        if (!found)
        {
            throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
        }
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()
    };

    const VpProfileProperties profile = { VP_KHR_ROADMAP_2022_NAME, VP_KHR_ROADMAP_2022_SPEC_VERSION };

    VpInstanceCreateInfo vpInstanceCreateInfo{
        .pCreateInfo = reinterpret_cast<const VkInstanceCreateInfo*>(&createInfo),
        .enabledFullProfileCount = 1,
        .pEnabledFullProfiles = &profile
    };

    VkInstance rawInstance;
    if (vpCreateInstance(&vpInstanceCreateInfo, nullptr, &rawInstance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan Instance with Profile support!");
    }

    instance = vk::raii::Instance(context, rawInstance);
}

void VulkanContext::setupDebugMessenger()
{
    // Always set up the debug messenger
    // It will only be used if validation layers are enabled via vulkanconfig

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback
    };

    try
    {
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }
    catch (const vk::SystemError& err)
    {
        // If the debug utils extension is not available, this will fail
        // That's okay; it just means validation layers aren't enabled
        std::cout << "Debug messenger not available. Validation layers may not be enabled." << std::endl;
    }
}

std::vector<const char*> VulkanContext::getRequiredExtensions() const
{
    // Get the required extensions from GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Check if the debug utils extension is available
    std::vector<vk::ExtensionProperties> props = context.enumerateInstanceExtensionProperties();
    bool debugUtilsAvailable = std::ranges::any_of(props,
        [](vk::ExtensionProperties const& ep) {
            return strcmp(ep.extensionName, vk::EXTDebugUtilsExtensionName) == 0;
        });

    // Always include the debug utils extension if available
    if (debugUtilsAvailable)
    {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    else
    {
        std::cout << "VK_EXT_debug_utils extension not available. Validation layers may not work." << std::endl;
    }

    return extensions;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanContext::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*)
{
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
    {
        std::cerr << "validation layer: type " << vk::to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
    }

    return vk::False;
}