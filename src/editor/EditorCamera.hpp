#pragma once
#include "scene/Camera.hpp"

class Timestep;
class Event;

class EditorCamera : public Camera
{
public:
	EditorCamera();

	~EditorCamera() = default;

	void OnUpdate(Timestep ts);
	void OnEvent(Event& e);

	const glm::vec3& GetPosition() const { return m_Position; }
	glm::quat GetOrientation() const;

	glm::vec3 GetUpDirection() const;
	glm::vec3 GetRightDirection() const;
	glm::vec3 GetForwardDirection() const;

private:
	void UpdateCameraView();

private:
	glm::vec3 m_Position = { 0.0f, 0.0f, 5.0f };

	float m_Pitch = 0.0f, m_Yaw = 0.0f;

	float m_MoveSpeed = 5.0f;
	float m_TurnSpeed = 1.5f;
};