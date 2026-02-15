#pragma once

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class VulkanRenderer;

class VulkanWindow
{
public:
    VulkanWindow();
    ~VulkanWindow();

    GLFWwindow* getWindow() const;
    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance) const;

    bool shouldClose() const;
    void pollEvents() const;
    void waitEvents() const;
    void getFramebufferSize(int& width, int& height) const;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void setUserPointer(VulkanRenderer* renderer);

private:
    GLFWwindow* window = nullptr;

    void initWindow();
    void cleanup();
};