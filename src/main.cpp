#include "renderer/VulkanRenderer.hpp"
#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/Components.hpp"
#include "core/Input.hpp"
#include "scene/AssetManager.hpp"
#include "editor/EditorUI.hpp"
#include "scene/Model.hpp"
#include "scene/SceneSerializer.hpp"


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

class GameLayer : public Layer
{
public:
	GameLayer() : Layer("Game Layer") 
	{
		m_CurrentScene = Application::Get().GetActiveScene();
	}
	~GameLayer() = default;
	virtual void OnAttach() override 
	{
		BindCameraControllerToPrimaryCamera();
	}
	virtual void OnDetach() override {}
	virtual void OnUpdate(Timestep ts) override {}
	virtual void OnImGuiRender() override {}
	virtual void OnEvent(Event& event) override 
	{
		std::cout << "GameLayer received event: " << event.GetName() << std::endl;
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<SceneLoadedEvent>([this](SceneLoadedEvent& event) { return OnSceneLoaded(event); });
	}

private:
	bool OnSceneLoaded(SceneLoadedEvent& e)
	{
		std::cout << "GameLayer handling SceneLoadedEvent for scene: " << std::endl;
		m_CurrentScene = Application::Get().GetActiveScene();
		BindCameraControllerToPrimaryCamera();
		return false;
	}

	void BindCameraControllerToPrimaryCamera()
	{
		auto view = m_CurrentScene->m_Registry.view<CameraComponent>();
		for (auto entityID : view)
		{
			Entity cameraEntity{ entityID, m_CurrentScene.get()};
			// If it's the primary camera, give it the controller
			if (cameraEntity.GetComponent<CameraComponent>().Primary)
			{
				cameraEntity.AddComponent<NativeScriptComponent>().Bind<CameraController>();
				std::cout << "Camera Controller successfully attached!\n";
				break;
			}
		}
	}
private:
	std::shared_ptr<Scene> m_CurrentScene = nullptr;
};

class EditorLayer : public Layer
{
	public:
	EditorLayer(AssetManager* assetManager, VulkanRenderer* renderer) 
		: Layer("Editor Layer"), 
		m_AssetManager(assetManager), 
		m_Renderer(renderer) {}

	~EditorLayer() = default;
	virtual void OnAttach() override {}
	virtual void OnDetach() override {}
	virtual void OnUpdate(Timestep ts) override {}
	virtual void OnImGuiRender() override 
	{
		std::shared_ptr<Scene> activeScene = Application::Get().GetActiveScene();
		m_EditorUI.Draw(*activeScene, *m_AssetManager, *m_Renderer);
	}

	virtual void OnEvent(Event& event) override {}

private:
	EditorUI m_EditorUI;
	AssetManager* m_AssetManager = nullptr;
	VulkanRenderer* m_Renderer = nullptr;
};

class Sandbox : public Application
{
	public:
	Sandbox() : Application("Vulkan Engine Sandbox") 
	{
		PushLayer(new GameLayer());
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