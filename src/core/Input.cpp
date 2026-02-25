#include "Input.hpp"


GLFWwindow* Input::s_Window = nullptr;

void Input::Init(GLFWwindow* window)
{
    s_Window = window;
}

bool Input::IsKeyPressed(KeyCode key)
{
    if (!s_Window) return false;
    auto state = glfwGetKey(s_Window, static_cast<int>(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(int button)
{
    if (!s_Window) return false;
    auto state = glfwGetMouseButton(s_Window, button);
    return state == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition()
{
    if (!s_Window) return { 0.0f, 0.0f };
    double xpos, ypos;
    glfwGetCursorPos(s_Window, &xpos, &ypos);
    return { (float)xpos, (float)ypos };
}

float Input::GetMouseX()
{
    return GetMousePosition().x;
}

float Input::GetMouseY()
{
    return GetMousePosition().y;
}