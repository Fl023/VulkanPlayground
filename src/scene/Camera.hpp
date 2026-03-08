#pragma once


class Camera
{
public:
	Camera(const std::string& name = "default");
	~Camera() = default;

	enum class ProjectionType { Perspective = 0, Orthographic = 1 };

	void SetPerspective(float verticalFOV, float aspectRatio, float nearClip, float farClip);
	void SetOrthographic(float size, float nearClip, float farClip);

	void SetViewportSize(uint32_t width, uint32_t height);

	const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
	const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
	const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

	float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
	float GetOrthographicSize() const { return m_OrthographicSize; }
	float GetAspectRatio() const { return m_AspectRatio; }
	float GetNearClip() const { return m_NearClip; }
	float GetFarClip() const { return m_FarClip; }

	ProjectionType GetProjectionType() const { return m_ProjectionType; }
	void SetProjectionType(ProjectionType type) {
		m_ProjectionType = type;
		RecalculateProjection();
	}

	void RecalculateViewMatrix(const glm::mat4& transform);

	

private:
	void RecalculateProjection();

private:
	std::string m_Name;
	// Projection properties
	
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