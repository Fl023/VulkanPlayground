#pragma once

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class VulkanRenderer;
class Event;

class VulkanWindow
{
public:
    using EventCallbackFn = std::function<void(std::unique_ptr<Event>)>;

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

    void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

private:
    void initWindow(uint32_t width, uint32_t height, const std::string& title);
    void cleanup();
private:
    GLFWwindow* window = nullptr;

    struct WindowData
    {
        std::string Title;
        unsigned int Width, Height;
        EventCallbackFn EventCallback;
    };

    WindowData m_Data;
};