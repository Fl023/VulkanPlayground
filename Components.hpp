#pragma once

#include "Mesh.hpp" 

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
    std::shared_ptr<Mesh> MeshAsset;

    MeshComponent() = default;
    MeshComponent(std::shared_ptr<Mesh> mesh) : MeshAsset(std::move(mesh)) {}
};
