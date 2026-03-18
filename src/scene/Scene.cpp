#include "Scene.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "core/Input.hpp"

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(m_Registry.create(), this);

    entity.AddComponent<TransformComponent>();
    entity.AddComponent<RelationshipComponent>();
    auto& tag = entity.AddComponent<TagComponent>();
    tag.Tag = name.empty() ? "Entity" : name;

    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    m_Registry.destroy(entity);
}

void Scene::UpdateTransformHierarchy(entt::entity entityHandle, const glm::mat4& parentMatrix)
{
    auto& transform = m_Registry.get<TransformComponent>(entityHandle);

    // Calculate World = ParentWorldMatrix * LocalMatrix
    transform.WorldMatrix = parentMatrix * transform.GetLocalTransform();

    // Recurse to children
    if (m_Registry.all_of<RelationshipComponent>(entityHandle)) {
        entt::entity current_child = m_Registry.get<RelationshipComponent>(entityHandle).FirstChild;

        while (current_child != entt::null) {
            UpdateTransformHierarchy(current_child, transform.WorldMatrix);
            current_child = m_Registry.get<RelationshipComponent>(current_child).NextSibling;
        }
    }
}

void Scene::OnUpdate(float deltaTime)
{
    auto relView = m_Registry.view<TransformComponent, RelationshipComponent>();

    for (auto entityHandle : relView) {
        const auto& rel = relView.get<RelationshipComponent>(entityHandle);

        // We only want to start the recursion from ROOT nodes (nodes with no parent).
        // The recursive function will naturally reach all the children.
        if (rel.Parent == entt::null) {
            UpdateTransformHierarchy(entityHandle, glm::mat4(1.0f)); // Root nodes have an Identity parent matrix
        }
    }
}


