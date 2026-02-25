#include "renderer/VulkanRenderer.hpp"
#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/Components.hpp"


class HelloTriangleApplication
{
public:
	void run()
	{
		initScene();
		mainLoop();
		cleanup();
	}

private:
	VulkanRenderer renderer;
	Scene activeScene;
	Camera mainCamera;

	void initScene()
	{
		Entity cameraEntity = activeScene.CreateEntity("MainCamera");
		float aspect = (float)renderer.getWindow().getWidth() / (float)renderer.getWindow().getHeight();
		mainCamera.SetPerspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
		auto& cam = cameraEntity.AddComponent<CameraComponent>(mainCamera);
		auto& trans = cameraEntity.GetComponent<TransformComponent>();

		trans.Translation = glm::vec3(2.0f, 2.0f, 2.0f);

		glm::vec3 direction = glm::normalize(glm::vec3(0.0f) - trans.Translation);
		trans.Rotation = glm::eulerAngles(glm::quatLookAt(direction, glm::vec3(0.0f, 0.0f, 1.0f)));


		auto myMesh = std::make_shared<Mesh>(renderer.getDevice(), vertices, indices);
		auto myTexture = std::make_shared<Texture>(renderer.getDevice(), "resources/statue.jpg");
		std::shared_ptr<Material> statueMaterial = renderer.createMaterial(myTexture);

		// --- ERSTES OBJEKT ---
		Entity triangle1 = activeScene.CreateEntity("MainTriangle");
		triangle1.AddComponent<MeshComponent>(myMesh);
		triangle1.AddComponent<MaterialComponent>(statueMaterial);

		// Wir holen das Transform und schieben es nach links
		auto& trans1 = triangle1.GetComponent<TransformComponent>();
		trans1.Translation = glm::vec3(-1.0f, 0.0f, 0.0f);


		// --- ZWEITES OBJEKT ---
		Entity triangle2 = activeScene.CreateEntity("SecondTriangle");
		triangle2.AddComponent<MeshComponent>(myMesh);
		triangle2.AddComponent<MaterialComponent>(statueMaterial);

		// Wir holen das Transform und schieben es nach rechts
		auto& trans2 = triangle2.GetComponent<TransformComponent>();
		trans2.Translation = glm::vec3(1.0f, 0.0f, 0.0f);

		// --- ZWEITES OBJEKT ---
		Entity triangle3 = activeScene.CreateEntity("SecondTriangle");
		triangle3.AddComponent<MeshComponent>(myMesh);
		triangle3.AddComponent<MaterialComponent>(statueMaterial);

		// Wir holen das Transform und schieben es nach rechts
		auto& trans3 = triangle3.GetComponent<TransformComponent>();
		trans3.Translation = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	void mainLoop()
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		while (!renderer.getWindow().shouldClose())
		{
			renderer.getWindow().pollEvents();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			auto view = activeScene.m_Registry.view<TagComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& tag = view.get<TagComponent>(entity);
				if (tag.Tag == "MainTriangle")
				{
					auto& trans = view.get<TransformComponent>(entity);
					trans.Rotation.z = time * glm::radians(90.0f);
				}
			}

			renderer.drawFrame(activeScene);
		}
		renderer.getDevice().getDevice().waitIdle();
	}

	void cleanup()
	{
	}
};

int main()
{
	try
	{
		HelloTriangleApplication app;
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}