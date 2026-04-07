#pragma once

#include "events/Event.hpp"
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
    void OnUpdateEditor(float deltaTime);
    void OnStop();

	void OnEvent(Event& event);
    void OnViewportResize(uint32_t width, uint32_t height);

	void Clear();

    void SetPrimaryCamera(Entity cameraEntity);
    Entity GetPrimaryCameraEntity();

    // Wir machen die Registry public für das RenderSystem (einfacher für den Anfang)
    entt::registry m_Registry;

private:
    friend class Entity;

    void UpdateTransformHierarchy(entt::entity entityHandle, const glm::mat4& parentMatrix);

private:
    uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
};