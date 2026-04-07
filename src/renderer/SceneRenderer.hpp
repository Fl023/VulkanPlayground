#pragma once

class VulkanRenderer;
class AssetManager;
class RenderGraph;
class Scene;
class Camera;
#include "RenderGraph.hpp"
#include "scene/RenderView.hpp"
#include "scene/AssetHandle.hpp"

class SceneRenderer {
public:
    virtual ~SceneRenderer() = default;

    // Initialize passes, register virtual textures, etc.
    virtual void Init(VulkanRenderer* backend, AssetManager* assetManager) {
        m_Backend = backend;
        m_AssetManager = assetManager;
    }

    // Extract data and build the graph for this specific frame
    virtual void RenderScene(Scene& scene, const Camera& activeCamera) = 0;

protected:
    VulkanRenderer* m_Backend = nullptr;
    AssetManager* m_AssetManager = nullptr;
};


class DefaultSceneRenderer : public SceneRenderer {
public:
    void Init(VulkanRenderer* backend, AssetManager* assetManager) override;

    void RenderScene(Scene& scene, const Camera& activeCamera) override;

private:
    RenderGraph m_Graph;
    RenderView ExtractRenderData(Scene& scene);

    AssetHandle m_SkyboxMaterial = INVALID_ASSET_HANDLE;
};