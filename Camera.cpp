#include "Camera.hpp"

Camera::Camera()
{
	RecalculateProjection();
}

void Camera::SetPerspective(float verticalFOV, float aspectRatio, float nearClip, float farClip)
{
	m_ProjectionType = ProjectionType::Perspective;
	m_PerspectiveFOV = verticalFOV;
	m_AspectRatio = aspectRatio;
	m_NearClip = nearClip;
	m_FarClip = farClip;
	RecalculateProjection();
}

void Camera::SetOrthographic(float size, float nearClip, float farClip)
{
	m_ProjectionType = ProjectionType::Orthographic;
	m_OrthographicSize = size;
	m_NearClip = nearClip;
	m_FarClip = farClip;
	RecalculateProjection();
}

void Camera::SetViewportSize(uint32_t width, uint32_t height)
{
	m_AspectRatio = (float)width / (float)height;
	RecalculateProjection();
}

void Camera::RecalculateProjection()
{
	if (m_ProjectionType == ProjectionType::Perspective)
	{
		m_ProjectionMatrix = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_NearClip, m_FarClip);
		m_ProjectionMatrix[1][1] *= -1;
	}
	else // Orthographic
	{
		float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
		float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
		float orthoBottom = -m_OrthographicSize * 0.5f;
		float orthoTop = m_OrthographicSize * 0.5f;
		m_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_NearClip, m_FarClip);
		m_ProjectionMatrix[1][1] *= -1;
	}
}

void Camera::RecalculateViewMatrix(const glm::mat4& transform)
{
	// The view matrix is the inverse of the camera's transform matrix.
	// It transforms the world so that it is viewed from the camera's perspective.
	m_ViewMatrix = glm::inverse(transform);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}