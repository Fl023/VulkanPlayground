#pragma once

#include <entt/entt.hpp>

class Entity; // Forward Declaration

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    // Erstellt eine Entity und fügt Standard-Komponenten hinzu
    Entity CreateEntity(const std::string& name = std::string());
    void DestroyEntity(Entity entity);

    void OnStart();
    void OnUpdate(float deltaTime);
    void OnStop();

    

	void Clear();

    // Wir machen die Registry public für das RenderSystem (einfacher für den Anfang)
    entt::registry m_Registry;

private:
    friend class Entity;

    void UpdateTransformHierarchy(entt::entity entityHandle, const glm::mat4& parentMatrix);
};