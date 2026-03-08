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
		: mainWindow(1280, 720, "Vulkan Engine"),
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
	Entity m_SelectedEntity;

	AssetManager assetManager;
	EditorUI editorUI;

	void initScene()
	{
		// 1. --- DEFAULT ASSETS ERSTELLEN ---

		// Weiße Default-Textur im RAM erstellen
		uint32_t whitePixel = 0xFFFFFFFF;
		auto defaultTex = std::make_shared<Texture>(renderer.getDevice(), 1, 1, &whitePixel);
		assetManager.AddTexture("Default White", defaultTex);

		// Default-Material erstellen
		auto defaultMat = renderer.createMaterial("Default Material", defaultTex);
		assetManager.AddMaterial("Default Material", defaultMat);

		renderer.SetDefaultMaterial(defaultMat);


		Entity cameraEntity = activeScene.CreateEntity("MainCamera");
		float aspect = (float)renderer.getWindow().getWidth() / (float)renderer.getWindow().getHeight();
		mainCamera.SetPerspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
		auto& cam = cameraEntity.AddComponent<CameraComponent>(mainCamera);
		auto& trans = cameraEntity.GetComponent<TransformComponent>();

		trans.Translation = glm::vec3(2.0f, 2.0f, 2.0f);

		glm::vec3 direction = glm::normalize(glm::vec3(0.0f) - trans.Translation);
		trans.Rotation = glm::eulerAngles(glm::quatLookAt(direction, glm::vec3(0.0f, 0.0f, 1.0f)));

		auto squareMesh = std::make_shared<Mesh>(renderer.getDevice(), "Square", squareVertices, squareIndices);
		assetManager.AddMesh("Square", squareMesh);
		auto myMesh = std::make_shared<Mesh>(renderer.getDevice(), "Cube", cubeVertices, cubeIndices);
		assetManager.AddMesh("Cube", myMesh);
		auto myTexture = std::make_shared<Texture>(renderer.getDevice(), "resources/statue.jpg");
		assetManager.AddTexture("Statue Texture", myTexture);
		std::shared_ptr<Material> statueMaterial = renderer.createMaterial("Statue Material", myTexture);
		assetManager.AddMaterial("Statue Material", statueMaterial);

		// --- ERSTES OBJEKT ---
		Entity triangle1 = activeScene.CreateEntity("MainTriangle");
		triangle1.AddComponent<MeshComponent>(assetManager.GetMeshes().at("Cube"));
		triangle1.AddComponent<MaterialComponent>(statueMaterial);

		// Wir holen das Transform und schieben es nach links
		auto& trans1 = triangle1.GetComponent<TransformComponent>();
		trans1.Translation = glm::vec3(-1.0f, 0.0f, 0.0f);


		// --- ZWEITES OBJEKT ---
		Entity triangle2 = activeScene.CreateEntity("SecondTriangle");
		triangle2.AddComponent<MeshComponent>(assetManager.GetMeshes().at("Cube"));
		triangle2.AddComponent<MaterialComponent>(statueMaterial);

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
			renderer.drawFrame(activeScene);

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