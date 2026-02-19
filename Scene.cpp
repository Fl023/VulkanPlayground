#include "Scene.hpp"
#include "Entity.hpp"
#include "Components.hpp"

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(m_Registry.create(), this);

    // Jede Entity hat standardm‰ﬂig ein Transform und ein Tag
    entity.AddComponent<TransformComponent>();
    auto& tag = entity.AddComponent<TagComponent>();
    tag.Tag = name.empty() ? "Entity" : name;

    return entity;
}