#pragma once

#include "scene/AssetHandle.hpp"

// A single, abstracted draw command
struct RenderDrawCall {
    AssetHandle MeshHandle;
    glm::mat4 Transform;
};

struct GlobalFrameData {
    alignas(16) glm::mat4 ViewMatrix;
    alignas(16) glm::mat4 ProjMatrix;
};

struct PushConstants {
    alignas(16) glm::mat4 modelMatrix;
    uint32_t textureIndex;
};

// The complete visual state of the frame
struct RenderView {
    GlobalFrameData FrameData;
    
    // Environment
    AssetHandle ActiveSkybox = INVALID_ASSET_HANDLE;

    // The "Command Bucket" - Sorted by Pipeline -> Material -> Meshes
    // Note: You might want to use unordered_map or a custom flat_map for AAA performance later, 
    // but std::map is great for getting it working!
    std::map<class VulkanPipeline*, std::map<uint32_t, std::vector<RenderDrawCall>>> OpaqueBucket;

    void Clear() {
		FrameData = {};
		OpaqueBucket.clear();
        ActiveSkybox = INVALID_ASSET_HANDLE;
    }
};