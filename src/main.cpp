#include "renderer/VulkanRenderer.hpp"
#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/Components.hpp"
#include "core/Input.hpp"


class HelloTriangleApplication
{
public:
	HelloTriangleApplication()
		: mainWindow(1280, 720, "Vulkan Engine"), // Angenommen, dein Fenster hat Konstruktor-Parameter
		renderer(mainWindow) // Hier wird das Fenster �bergeben!
	{
		Input::Init(mainWindow.getNativeWindow());
	}

	void run()
	{
		initScene();
		mainLoop();
		cleanup();
	}

private:
	VulkanWindow mainWindow;
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

		static auto lastFrameTime = startTime;

		while (!renderer.getWindow().shouldClose())
		{
			renderer.getWindow().pollEvents();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
			float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
			lastFrameTime = currentTime;

			auto view = activeScene.m_Registry.view<TagComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& tag = view.get<TagComponent>(entity);
				if (tag.Tag == "MainTriangle")
				{
					auto& trans = view.get<TransformComponent>(entity);
					//trans.Rotation.z = deltaTime * glm::radians(90.0f);

					float speed = 0.5f * deltaTime;

					if (Input::IsKeyPressed(KeyCode::W)) trans.Translation.z -= speed;
					if (Input::IsKeyPressed(KeyCode::S)) trans.Translation.z += speed;
					if (Input::IsKeyPressed(KeyCode::A)) trans.Translation.x -= speed;
					if (Input::IsKeyPressed(KeyCode::D)) trans.Translation.x += speed;
				}
			}

			// --- NEU: 1. ImGui Frame starten ---
			renderer.beginUI();

			// --- NEU: 2. UI definieren ---
			// (Durch Viewports kannst du dieses Fenster später einfach aus dem Hauptfenster rausziehen!)
			ImGui::Begin("Engine Debugger");
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::Text("DeltaTime: %.3f ms", deltaTime * 1000.0f);
			ImGui::End();

			// --- 3. Szene & UI auf die GPU schicken ---
			renderer.drawFrame(activeScene);

			// --- NEU: 4. Externe ImGui Fenster (Viewports) updaten ---
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
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