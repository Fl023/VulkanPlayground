#pragma once

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class VulkanRenderer;

class VulkanWindow
{
public:
    VulkanWindow(uint32_t width, uint32_t height, const std::string& title);
    ~VulkanWindow();

    GLFWwindow* getWindow() const;
    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance) const;

    bool shouldClose() const;
    void pollEvents() const;
    void waitEvents() const;
    void getFramebufferSize(int& width, int& height) const;
	float getWidth() const;
	float getHeight() const;
    GLFWwindow* getNativeWindow() const { return window; }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void setUserPointer(VulkanRenderer* renderer);

private:
    GLFWwindow* window = nullptr;

    void initWindow(uint32_t width, uint32_t height, const std::string& title);
    void cleanup();
};