#include "EditorCamera.hpp"
#include "core/Input.hpp"
#include "events/Event.hpp"
#include "core/Timestep.hpp"

EditorCamera::EditorCamera()
    : Camera("EditorCamera")
{
    // Set a good default view for the editor
    SetPerspective(glm::radians(45.0f), 1.778f, 0.1f, 1000.0f);
    UpdateCameraView();
}

void EditorCamera::OnUpdate(Timestep ts)
{
    // ==========================================
    // 1. PROCESS ROTATION (Arrow Keys)
    // ==========================================
    if (Input::IsKeyPressed(KeyCode::Left))  m_Yaw += m_TurnSpeed * ts;
    if (Input::IsKeyPressed(KeyCode::Right)) m_Yaw -= m_TurnSpeed * ts;
    if (Input::IsKeyPressed(KeyCode::Up))    m_Pitch += m_TurnSpeed * ts;
    if (Input::IsKeyPressed(KeyCode::Down))  m_Pitch -= m_TurnSpeed * ts;

    // Clamp the pitch to prevent flipping upside down
    m_Pitch = glm::clamp(m_Pitch, glm::radians(-89.0f), glm::radians(89.0f));

    // ==========================================
    // 2. CALCULATE LOCAL VECTORS
    // ==========================================
    // These use the GetOrientation() quaternion built from m_Pitch and m_Yaw
    glm::vec3 forward = GetForwardDirection();
    glm::vec3 right = GetRightDirection();
    glm::vec3 up = GetUpDirection();

    // ==========================================
    // 3. PROCESS TRANSLATION (WASD + EQ)
    // ==========================================
    glm::vec3 movement(0.0f);

    if (Input::IsKeyPressed(KeyCode::W)) movement += forward;
    if (Input::IsKeyPressed(KeyCode::S)) movement -= forward;
    if (Input::IsKeyPressed(KeyCode::D)) movement += right;
    if (Input::IsKeyPressed(KeyCode::A)) movement -= right;
    if (Input::IsKeyPressed(KeyCode::E)) movement += up;
    if (Input::IsKeyPressed(KeyCode::Q)) movement -= up;

    // Normalize so diagonal movement isn't faster, then apply to m_Position
    if (glm::length(movement) > 0.0f)
    {
        m_Position += glm::normalize(movement) * m_MoveSpeed * (float)ts;
    }

    // ==========================================
    // 4. UPDATE THE CAMERA MATH
    // ==========================================
    UpdateCameraView();
}

glm::quat EditorCamera::GetOrientation() const
{
    // Pitch is X axis, Yaw is Y axis
    return glm::quat(glm::vec3(m_Pitch, m_Yaw, 0.0f));
}

glm::vec3 EditorCamera::GetForwardDirection() const
{
    return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 EditorCamera::GetRightDirection() const
{
    return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 EditorCamera::GetUpDirection() const
{
    return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

void EditorCamera::UpdateCameraView()
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(GetOrientation());
    RecalculateViewMatrix(transform);
}

void EditorCamera::OnEvent(Event& e)
{
    // You can wire up mouse scrolling here later to change movement speed!
}