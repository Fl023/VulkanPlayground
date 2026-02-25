#pragma once

class VulkanContext
{
public:
    VulkanContext();
    ~VulkanContext();

    const vk::raii::Instance& getInstance() const;

private:
    void createInstance();
    void setupDebugMessenger();
    std::vector<const char*> getRequiredExtensions() const;

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

private:
    vk::raii::Context  context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
};