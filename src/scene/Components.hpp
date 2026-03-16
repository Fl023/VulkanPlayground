#pragma once

#include "AssetHandle.hpp"
#include "Mesh.hpp" 
#include "Camera.hpp"
#include "Texture.hpp"
#include "Material.hpp"

struct TagComponent {
    std::string Tag;

    TagComponent() = default;
    TagComponent(const std::string& tag) : Tag(tag) {}
};

struct TransformComponent {
    glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f }; // In Radiant (Euler-Winkel)
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

    TransformComponent() = default;

    // Berechnet die Model-Matrix für Push Constants
    glm::mat4 GetTransform() const {
        glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

        return glm::translate(glm::mat4(1.0f), Translation)
            * rotation
            * glm::scale(glm::mat4(1.0f), Scale);
    }
};

struct MeshComponent {
    AssetHandle MeshHandle = INVALID_ASSET_HANDLE;

    MeshComponent() = default;
    MeshComponent(AssetHandle handle) : MeshHandle(handle) {}
};

struct MaterialComponent {
    AssetHandle MaterialHandle = INVALID_ASSET_HANDLE;

    MaterialComponent() = default;
    MaterialComponent(AssetHandle handle) : MaterialHandle(handle) {}
};

struct CameraComponent {
    Camera SceneCamera;
    bool Primary = true;
    bool FixedAspectRatio = false;

    CameraComponent() = default;
    CameraComponent(const Camera& camera)
        : SceneCamera(camera) {
    }
};

struct SkyboxComponent {
    AssetHandle CubemapHandle = INVALID_ASSET_HANDLE;

    SkyboxComponent() = default;
    SkyboxComponent(AssetHandle handle) : CubemapHandle(handle) {}
};

