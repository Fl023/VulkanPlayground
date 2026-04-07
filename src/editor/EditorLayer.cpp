#include "EditorLayer.hpp"
#include "scene/Components.hpp"
#include "scene/SceneSerializer.hpp"
#include "scripts/NativeScriptsRegistry.hpp"


EditorLayer::EditorLayer(AssetManager* assetManager, VulkanRenderer* renderer)
    : Layer("Editor Layer"), m_AssetManager(assetManager), m_Renderer(renderer)
{
}

void EditorLayer::OnAttach()
{
    m_SceneRenderer = std::make_unique<DefaultSceneRenderer>();
    m_SceneRenderer->Init(m_Renderer, m_AssetManager);

    m_EditorScene = std::make_shared<Scene>();
    OpenScene("defaultScene.yaml");

    m_ActiveScene = m_EditorScene;
}

void EditorLayer::OnDetach()
{
    m_SceneRenderer.reset();
}

void EditorLayer::OnUpdate(Timestep deltaTime)
{
    glm::vec2 panelSize = m_EditorUI.GetViewportSize();
    if (m_ViewportSize != panelSize && panelSize.x > 0.0f && panelSize.y > 0.0f)
    {
        m_ViewportSize = panelSize;
        m_EditorCamera.SetViewportSize(panelSize.x, panelSize.y);
    }

    if (m_SceneState == SceneState::Play)
    {
        // Run the full game loop (Physics, Scripts, and Transforms)
        m_ActiveScene->OnUpdate(deltaTime);
		Entity primaryCamera = m_ActiveScene->GetPrimaryCameraEntity();
        if (primaryCamera) 
        {
            auto& cameraComp = primaryCamera.GetComponent<CameraComponent>();
            auto& transformComp = primaryCamera.GetComponent<TransformComponent>();
            cameraComp.SceneCamera.RecalculateViewMatrix(transformComp.GetLocalTransform());
            m_SceneRenderer->RenderScene(*m_ActiveScene, cameraComp.SceneCamera);
		}
        
    }
    else if (m_SceneState == SceneState::Edit)
    {
        // Run ONLY the mathematical updates needed for the Editor UI
        m_ActiveScene->OnUpdateEditor(deltaTime);
		m_EditorCamera.OnUpdate(deltaTime);
        m_SceneRenderer->RenderScene(*m_ActiveScene, m_EditorCamera);
    }
}

void EditorLayer::OnImGuiRender()
{
    // 1. Draw the Toolbar and pass the Play/Stop callbacks
    m_EditorUI.DrawToolbar(
        m_SceneState,
        [this]() { OnScenePlay(); },
        [this]() { OnSceneStop(); }
    );

    // 2. Draw the main Editor UI and pass the Open Scene callback
    // Notice how we ALWAYS pass m_ActiveScene to the UI, so it renders the live game!
    m_EditorUI.Draw(*m_ActiveScene, *m_AssetManager, *m_Renderer, [this](const std::string& filepath)
    {
        OpenScene(filepath);
    });
}

void EditorLayer::OnEvent(Event& event)
{
    // Pass events down to the active scene (useful for Input Events if your scripts use them!)
    if (m_SceneState == SceneState::Play && m_ActiveScene)
    {
        m_ActiveScene->OnEvent(event);
    }
}

void EditorLayer::OnScenePlay()
{
    m_SceneState = SceneState::Play;

    // 1. Create a fresh runtime scene
    m_ActiveScene = std::make_shared<Scene>();

    // 2. Deep copy the Editor scene into the Runtime scene using our RAM-to-RAM serializer trick!
    SceneSerializer serializer(m_EditorScene.get(), m_AssetManager);
    std::string sceneData = serializer.SerializeToString();

    SceneSerializer deserializer(m_ActiveScene.get(), m_AssetManager);
    deserializer.DeserializeFromString(sceneData, *m_Renderer);

    // 3. Start the game loop! (Allocates native scripts, fires Lua OnCreate)
    m_ActiveScene->OnStart();
    std::cout << "--- ENTERED PLAY MODE ---" << std::endl;
}

void EditorLayer::OnSceneStop()
{
    m_SceneState = SceneState::Edit;

    // 1. Trigger OnDestroy for scripts, safely clean up game memory
    m_ActiveScene->OnStop();

    // 2. Point the active scene back to the untouched Editor scene
    m_ActiveScene = m_EditorScene;
    std::cout << "--- RETURNED TO EDIT MODE ---" << std::endl;
}

void EditorLayer::OpenScene(const std::string& filepath)
{
    // Force the editor to stop playing if they load a file during Play Mode
    if (m_SceneState == SceneState::Play)
    {
        OnSceneStop();
    }

    // 1. Create a brand new scene
    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    // 2. Load the data from the hard drive
    SceneSerializer deserializer(newScene.get(), m_AssetManager);
    if (deserializer.Deserialize(filepath, *m_Renderer))
    {
        // 3. Update the Editor state
        m_EditorScene = newScene;
        m_ActiveScene = m_EditorScene;

        std::cout << "Successfully loaded scene: " << filepath << std::endl;
    }
    else
    {
        std::cerr << "Failed to load scene: " << filepath << std::endl;
    }
}