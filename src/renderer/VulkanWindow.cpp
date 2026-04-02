#include "VulkanWindow.hpp"
#include "VulkanRenderer.hpp" // Required to access VulkanRenderer members
#include "events/Event.hpp"
#include "events/ApplicationEvent.hpp"
#include "events/KeyEvent.hpp"
#include "events/MouseEvent.hpp"

VulkanWindow::VulkanWindow(uint32_t width, uint32_t height, const std::string& title)
{
    initWindow(width, height, title);
}

VulkanWindow::~VulkanWindow()
{
    cleanup();
}

void VulkanWindow::initWindow(uint32_t width, uint32_t height, const std::string& title)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    
    // --- The crucial link between GLFW and our C++ class ---
    glfwSetWindowUserPointer(window, &m_Data);

    // =================================================================
    // 3. GLFW CALLBACKS
    // =================================================================

    // --- Window Resize ---
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
    {
        // Retrieve the data struct
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        data.Width = width;
        data.Height = height;

        // Create the C++ event and pass it to the callback
        auto event = std::make_unique<WindowResizeEvent>(width, height);
        data.EventCallback(std::move(event));
    });

    // --- Window Close ---
    glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        auto event = std::make_unique<WindowCloseEvent>();
        data.EventCallback(std::move(event));
    });

    // --- Key Input ---
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        // Map GLFW actions to your specific Event classes
        switch (action)
        {
        case GLFW_PRESS:
        {
            auto event = std::make_unique<KeyPressedEvent>(static_cast<KeyCode>(key), false);
            data.EventCallback(std::move(event));
            break;
        }
        case GLFW_RELEASE:
        {
            auto event = std::make_unique<KeyReleasedEvent>(static_cast<KeyCode>(key));
            data.EventCallback(std::move(event));
            break;
        }
        case GLFW_REPEAT:
        {
            auto event = std::make_unique<KeyPressedEvent>(static_cast<KeyCode>(key), true);
            data.EventCallback(std::move(event));
            break;
        }
        }
    });

    // --- Key Typed (for text input) ---
    glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int keycode)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        auto event = std::make_unique<KeyTypedEvent>(static_cast<KeyCode>(keycode));
        data.EventCallback(std::move(event));
    });

    // --- Mouse Button Input ---
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        switch (action)
        {
        case GLFW_PRESS:
        {
            auto event = std::make_unique<MouseButtonPressedEvent>(static_cast<MouseCode>(button));
            data.EventCallback(std::move(event));
            break;
        }
        case GLFW_RELEASE:
        {
            auto event = std::make_unique<MouseButtonReleasedEvent>(static_cast<MouseCode>(button));
            data.EventCallback(std::move(event));
            break;
        }
        }
    });

    // --- Mouse Scroll ---
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        auto event = std::make_unique<MouseScrolledEvent>((float)xOffset, (float)yOffset);
        data.EventCallback(std::move(event));
    });

    // --- Mouse Movement ---
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xPos, double yPos)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        auto event = std::make_unique<MouseMovedEvent>((float)xPos, (float)yPos);
        data.EventCallback(std::move(event));
    });
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