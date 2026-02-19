#include "VulkanRenderer.hpp"
#include "Scene.hpp"
#include "Entity.hpp"
#include "Components.hpp"


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

	void initScene()
	{
		auto myMesh = std::make_shared<Mesh>(renderer.getDevice(), vertices, indices);

		Entity triangle = activeScene.CreateEntity("MainTriangle");
		triangle.AddComponent<MeshComponent>(myMesh);
	}

	void mainLoop()
	{
		while (!renderer.getWindow().shouldClose())
		{
			renderer.getWindow().pollEvents();
			renderer.drawFrame(activeScene);
		}
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