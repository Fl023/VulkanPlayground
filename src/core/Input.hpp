#pragma once

#include "KeyCodes.hpp"

struct GLFWwindow; 

class Input
{
public:
    // Einmalig beim Start der Engine aufrufen
    static void Init(GLFWwindow* window);

    // --- Keyboard ---
    static bool IsKeyPressed(KeyCode key);

    // --- Mouse ---
    static bool IsMouseButtonPressed(int button);
    static glm::vec2 GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();

private:
    static GLFWwindow* s_Window;
};