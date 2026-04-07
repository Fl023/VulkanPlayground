#include "renderer/VulkanRenderer.hpp"
#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/Components.hpp"
#include "core/Input.hpp"
#include "scene/AssetManager.hpp"
#include "editor/EditorUI.hpp"
#include "editor/EditorLayer.hpp"
#include "scene/Model.hpp"
#include "scene/SceneSerializer.hpp"
#include "scripts/NativeScriptsRegistry.hpp"


#include "core/Application.hpp"

class CameraController : public ScriptableEntity
{
public:
	float MoveSpeed = 5.0f;
	float TurnSpeed = 2.0f;

	void OnUpdate(float deltaTime) override
	{
		// 1. Grab the camera's transform
		auto& transform = GetComponent<TransformComponent>();

		// 4. Process Rotation (Arrow Keys for simplicity, or map to mouse!)
		glm::vec3 euler = transform.GetEulerAngles();

		if (Input::IsKeyPressed(KeyCode::Left))  euler.y += TurnSpeed * deltaTime;
		if (Input::IsKeyPressed(KeyCode::Right)) euler.y -= TurnSpeed * deltaTime;
		if (Input::IsKeyPressed(KeyCode::Up))    euler.x += TurnSpeed * deltaTime;
		if (Input::IsKeyPressed(KeyCode::Down))  euler.x -= TurnSpeed * deltaTime;

		euler.x = glm::clamp(euler.x, glm::radians(-89.0f), glm::radians(89.0f));
		transform.SetEulerAngles(euler);


		// 2. Calculate local movement vectors based on current rotation
		// This ensures 'W' moves forward in the direction you are looking!
		glm::vec3 forward = glm::rotate(transform.GetRotation(), glm::vec3(0.0f, 0.0f, -1.0f));
		glm::vec3 right = glm::rotate(transform.GetRotation(), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 up = glm::rotate(transform.GetRotation(), glm::vec3(0.0f, 1.0f, 0.0f));

		glm::vec3 movement(0.0f);

		// 3. Process Translation (Adjust Key::W etc. to match your engine's KeyCodes)
		if (Input::IsKeyPressed(KeyCode::W)) movement += forward;
		if (Input::IsKeyPressed(KeyCode::S)) movement -= forward;
		if (Input::IsKeyPressed(KeyCode::D)) movement += right;
		if (Input::IsKeyPressed(KeyCode::A)) movement -= right;
		if (Input::IsKeyPressed(KeyCode::E)) movement += up;
		if (Input::IsKeyPressed(KeyCode::Q)) movement -= up;

		// Normalize so diagonal movement isn't faster
		if (glm::length(movement) > 0.0f) {
			transform.Translation += glm::normalize(movement) * MoveSpeed * deltaTime;
		}
	}
};

class Sandbox : public Application
{
	public:
	Sandbox() : Application("Vulkan Engine Sandbox") 
	{
		NativeScriptRegistry::Register<CameraController>("CameraController");
		// PushLayer(new GameLayer());
		PushOverlay(new EditorLayer(&GetAssetManager(), &GetRenderer()));
	}
	~Sandbox() = default;
};

int main()
{
	try
	{
		Sandbox app;
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}