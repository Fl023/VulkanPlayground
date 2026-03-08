#include "Scene.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "core/Input.hpp"

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(m_Registry.create(), this);

    // Jede Entity hat standardm‰ﬂig ein Transform und ein Tag
    entity.AddComponent<TransformComponent>();
    auto& tag = entity.AddComponent<TagComponent>();
    tag.Tag = name.empty() ? "Entity" : name;

    return entity;
}

void Scene::OnUpdate(float deltaTime)
{
    auto view = m_Registry.view<TagComponent, TransformComponent>();

    for (auto entityID : view)
    {
        auto& tag = view.get<TagComponent>(entityID);

        // Unsere tempor‰re, hartkodierte Spiellogik
        if (tag.Tag == "MainTriangle")
        {
            auto& trans = view.get<TransformComponent>(entityID);
            float speed = 0.5f * deltaTime;

            // Die Tastenabfragen
            if (Input::IsKeyPressed(KeyCode::W)) trans.Translation.z -= speed;
            if (Input::IsKeyPressed(KeyCode::S)) trans.Translation.z += speed;
            if (Input::IsKeyPressed(KeyCode::A)) trans.Translation.x -= speed;
            if (Input::IsKeyPressed(KeyCode::D)) trans.Translation.x += speed;
        }
    }
}

void Scene::DestroyEntity(Entity entity) {
    m_Registry.destroy(entity);
}