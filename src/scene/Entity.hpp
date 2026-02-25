#pragma once

#include "Scene.hpp"
#include <entt/entt.hpp>

class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);
    Entity(const Entity& other) = default;

    // Komponente hinzufügen
    template<typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        // Sicherstellen, dass die Komponente nicht doppelt hinzugefügt wird
        assert(!HasComponent<T>() && "Entity already has component!");
        return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    // Komponente abrufen
    template<typename T>
    T& GetComponent()
    {
        assert(HasComponent<T>() && "Entity does not have component!");
        return m_Scene->m_Registry.get<T>(m_EntityHandle);
    }

    // Prüfen, ob Komponente existiert
    template<typename T>
    bool HasComponent()
    {
        return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
    }

    // Komponente entfernen
    template<typename T>
    void RemoveComponent()
    {
        assert(HasComponent<T>() && "Entity does not have component!");
        m_Scene->m_Registry.remove<T>(m_EntityHandle);
    }

    // Hilfskonvertierung: Prüfen, ob die Entity gültig ist
    operator bool() const { return m_EntityHandle != entt::null; }

    // Zugriff auf die rohe EnTT-ID (nützlich für Vergleiche)
    operator entt::entity() const { return m_EntityHandle; }

private:
    entt::entity m_EntityHandle{ entt::null };
    Scene* m_Scene = nullptr; // Die Scene, zu der diese Entity gehört [cite: 2, 3]
};