#include "VulkanRenderer.hpp"


class HelloTriangleApplication
{
public:
	void run()
	{
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	VulkanRenderer renderer;

	void initVulkan()
	{
	}

	void mainLoop()
	{
		while (!renderer.getWindow().shouldClose())
		{
			renderer.getWindow().pollEvents();
			renderer.drawFrame();
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