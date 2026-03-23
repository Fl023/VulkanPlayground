#include "renderer/VulkanRenderer.hpp"
#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/Components.hpp"
#include "core/Input.hpp"
#include "scene/AssetManager.hpp"
#include "editor/EditorUI.hpp"
#include "scene/ModelLoader.hpp"


class HelloTriangleApplication
{
public:
	HelloTriangleApplication()
		: mainWindow(1920, 1080, "Vulkan Engine"),
		renderer(mainWindow), mainCamera("Main Camera")
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

	AssetManager assetManager;
	EditorUI editorUI;

	void initScene()
	{
		Entity cameraEntity = activeScene.CreateEntity("MainCamera");
		float aspect = (float)renderer.getWindow().getWidth() / (float)renderer.getWindow().getHeight();
		mainCamera.SetPerspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
		auto& cam = cameraEntity.AddComponent<CameraComponent>(mainCamera);
		auto& trans = cameraEntity.GetComponent<TransformComponent>();

		trans.Translation = glm::vec3(2.0f, 2.0f, 2.0f);

		glm::vec3 direction = glm::normalize(glm::vec3(0.0f) - trans.Translation);
		trans.SetRotation(glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f)));


		assetManager.AddMesh("Cube", std::make_unique<Mesh>(renderer.getDevice(), "Cube", cubeVertices, cubeIndices));
		assetManager.AddMesh("Square", std::make_unique<Mesh>(renderer.getDevice(), "Square", squareVertices, squareIndices));


		std::array<std::string, 6> skyboxFaces = {
		"resources/skyboxes/Plants/posx.jpg", // +X
		"resources/skyboxes/Plants/negx.jpg", // -X
		"resources/skyboxes/Plants/posy.jpg", // +Y
		"resources/skyboxes/Plants/negy.jpg", // -Y
		"resources/skyboxes/Plants/posz.jpg", // +Z
		"resources/skyboxes/Plants/negz.jpg"  // -Z
		};

		// 1. Ask the AssetManager to load the 6 images into a single Cubemap texture
		AssetHandle skyboxHandle = assetManager.LoadCubemap(renderer, "DefaultSkybox", skyboxFaces);

		// 2. Create an entity to hold the skybox in the scene
		Entity skyboxEntity = activeScene.CreateEntity("Skybox");

		// 3. Attach the component and pass it the handle
		skyboxEntity.AddComponent<SkyboxComponent>(skyboxHandle);

		// 1. Load the Model Asset (Blueprint) into the AssetManager and upload to GPU
		AssetHandle adamModelHandle = assetManager.LoadModel(renderer, "AdamHead", "resources/adamHead/adamHead.gltf");

		// 2. Retrieve the loaded model blueprint from the vault
		Model* adamModelBlueprint = assetManager.GetModel(adamModelHandle);

		// 3. Instantiate the blueprint to create actual EnTT entities in your scene
		Entity adamHead = adamModelBlueprint->Instantiate(&activeScene);
		adamHead.GetComponent<TransformComponent>().SetEulerAngles(glm::vec3(0.0f, glm::radians(180.0f), 0.0f));
		adamHead.GetComponent<TransformComponent>().Scale = glm::vec3(0.5f);

	}

	void mainLoop()
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		static auto lastFrameTime = startTime;

		while (!renderer.getWindow().shouldClose())
		{
			renderer.getWindow().pollEvents();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
			lastFrameTime = currentTime;

			// --- 1. SPIELLOGIK ---
			activeScene.OnUpdate(deltaTime);

			// --- 2. UI ZEICHNEN ---
			renderer.beginUI();
			editorUI.Draw(activeScene, assetManager, renderer);

			// --- 3. RENDERN ---
			renderer.drawFrame(activeScene, assetManager);

			// --- VIEWPORTS UPDATEN ---
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}

		renderer.getDevice().getDevice().waitIdle();
		assetManager.Clear();
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