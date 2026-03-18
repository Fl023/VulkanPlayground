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

struct RelationshipComponent {
    entt::entity Parent = entt::null;
    entt::entity FirstChild = entt::null;
    entt::entity PrevSibling = entt::null;
    entt::entity NextSibling = entt::null;
};

struct TransformComponent {
    glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

    glm::mat4 WorldMatrix = glm::mat4(1.0f);

private:
    // Hiding these prevents other systems from breaking the sync!
    glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 EulerRotation = { 0.0f, 0.0f, 0.0f };

public:
    TransformComponent() = default;

    // --- GETTERS ---
    glm::quat GetRotation() const { return Rotation; }
    glm::vec3 GetEulerAngles() const { return EulerRotation; }

    // --- SETTERS (The Magic) ---
    void SetRotation(const glm::quat& q) {
        Rotation = q;
        EulerRotation = glm::eulerAngles(Rotation); // Auto-syncs the UI cache
    }

    void SetEulerAngles(const glm::vec3& euler) {
        EulerRotation = euler;
        Rotation = glm::quat(EulerRotation);        // Auto-syncs the math Quaternion
    }

    // --- MATRIX CALCULATIONS ---
    glm::mat4 GetLocalTransform() const {
        return glm::translate(glm::mat4(1.0f), Translation)
            * glm::toMat4(Rotation) // Safe to use Quat here
            * glm::scale(glm::mat4(1.0f), Scale);
    }

    void SetLocalTransform(const glm::mat4& matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::quat orientation;
        glm::decompose(matrix, Scale, orientation, Translation, skew, perspective);

        // Pass the decomposed orientation through our safe setter!
        SetRotation(orientation);
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

