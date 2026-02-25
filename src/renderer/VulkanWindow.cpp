#include "VulkanWindow.hpp"
#include "VulkanRenderer.hpp" // Required to access VulkanRenderer members

VulkanWindow::VulkanWindow()
{
    initWindow();
}

VulkanWindow::~VulkanWindow()
{
    cleanup();
}

void VulkanWindow::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanWindow::cleanup()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

GLFWwindow* VulkanWindow::getWindow() const
{
    return window;
}

vk::raii::SurfaceKHR VulkanWindow::createSurface(const vk::raii::Instance& instance) const
{
    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return vk::raii::SurfaceKHR(instance, rawSurface);
}

bool VulkanWindow::shouldClose() const
{
    return glfwWindowShouldClose(window);
}

void VulkanWindow::pollEvents() const
{
    glfwPollEvents();
}

void VulkanWindow::waitEvents() const
{
    glfwWaitEvents();
}

void VulkanWindow::getFramebufferSize(int& width, int& height) const
{
    glfwGetFramebufferSize(window, &width, &height);
}

float VulkanWindow::getWidth() const
{
    int width, height;
    getFramebufferSize(width, height);
	return static_cast<float>(width);
}

float VulkanWindow::getHeight() const
{
    int width, height;
    getFramebufferSize(width, height);
    return static_cast<float>(height);
}

void VulkanWindow::setUserPointer(VulkanRenderer* renderer)
{
    glfwSetWindowUserPointer(window, renderer);
}

void VulkanWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->framebufferResized = true;
    }
}