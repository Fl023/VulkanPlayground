#include "renderer/VulkanRenderer.hpp"
#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/Components.hpp"
#include "core/Input.hpp"
#include "scene/AssetManager.hpp"
#include "editor/EditorUI.hpp"


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
		trans.Rotation = glm::eulerAngles(glm::quatLookAt(direction, glm::vec3(0.0f, 0.0f, 1.0f)));

		// 1. Create Meshes as unique_ptrs and move them into the Vault. Capture the returned Handle!
		auto squareMesh = std::make_unique<Mesh>(renderer.getDevice(), "Square", squareVertices, squareIndices);
		AssetHandle squareHandle = assetManager.AddMesh("Square", std::move(squareMesh));

		auto myMesh = std::make_unique<Mesh>(renderer.getDevice(), "Cube", cubeVertices, cubeIndices);
		AssetHandle cubeHandle = assetManager.AddMesh("Cube", std::move(myMesh));

		// 2. Load the texture (this automatically returns an AssetHandle now)
		AssetHandle statueTexHandle = assetManager.LoadOrCreateTexture(renderer, "Statue Texture", "resources/statue.jpg");

		// 3. Create the Material directly inside the AssetManager. Capture the Handle!
		AssetHandle statueMatHandle = assetManager.CreateMaterial("Statue Material", statueTexHandle);

		// --- ERSTES OBJEKT ---
		Entity triangle1 = activeScene.CreateEntity("MainTriangle");
		// 4. Pass the integer handles directly to the components!
		triangle1.AddComponent<MeshComponent>(cubeHandle);
		triangle1.AddComponent<MaterialComponent>(statueMatHandle);

		// Wir holen das Transform und schieben es nach links
		auto& trans1 = triangle1.GetComponent<TransformComponent>();
		trans1.Translation = glm::vec3(-1.0f, 0.0f, 0.0f);

		// --- ZWEITES OBJEKT ---
		Entity triangle2 = activeScene.CreateEntity("SecondTriangle");
		triangle2.AddComponent<MeshComponent>(cubeHandle);
		triangle2.AddComponent<MaterialComponent>(statueMatHandle);

		// Wir holen das Transform und schieben es nach rechts
		auto& trans2 = triangle2.GetComponent<TransformComponent>();
		trans2.Translation = glm::vec3(1.0f, 0.0f, 0.0f);
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