#pragma once


class Camera
{
public:
	Camera();
	~Camera() = default;

	void SetPerspective(float verticalFOV, float aspectRatio, float nearClip, float farClip);
	void SetOrthographic(float size, float nearClip, float farClip);

	void SetViewportSize(uint32_t width, uint32_t height);

	const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
	const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
	const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

	// The view matrix is calculated based on an external transform (from a TransformComponent)
	void RecalculateViewMatrix(const glm::mat4& transform);

private:
	void RecalculateProjection();

private:
	// Projection properties
	enum class ProjectionType { Perspective = 0, Orthographic = 1 };
	ProjectionType m_ProjectionType = ProjectionType::Perspective;
	
	float m_PerspectiveFOV = glm::radians(45.0f);
	float m_OrthographicSize = 10.0f;

	float m_AspectRatio = 1.778f; // 16:9
	float m_NearClip = 0.1f;
	float m_FarClip = 1000.0f;

	// Resulting matrices
	glm::mat4 m_ProjectionMatrix{ 1.0f };
	glm::mat4 m_ViewMatrix{ 1.0f };
	glm::mat4 m_ViewProjectionMatrix{ 1.0f };
};