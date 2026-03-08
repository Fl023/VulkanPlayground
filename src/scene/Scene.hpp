#pragma once

#include <entt/entt.hpp>

class Entity; // Forward Declaration

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    // Erstellt eine Entity und fügt Standard-Komponenten hinzu
    Entity CreateEntity(const std::string& name = std::string());

    void OnUpdate(float deltaTime);

    void DestroyEntity(Entity entity);

    // Wir machen die Registry public für das RenderSystem (einfacher für den Anfang)
    entt::registry m_Registry;

private:
    friend class Entity;
};