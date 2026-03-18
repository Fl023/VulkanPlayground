#include "Entity.hpp"
#include "Components.hpp"

Entity::Entity(entt::entity handle, Scene* scene)
    : m_EntityHandle(handle), m_Scene(scene)
{
}

void Entity::AddChild(Entity child) {
    // Ensure both entities have a RelationshipComponent
    if (!HasComponent<RelationshipComponent>()) AddComponent<RelationshipComponent>();
    if (!child.HasComponent<RelationshipComponent>()) child.AddComponent<RelationshipComponent>();

    auto& parent_rel = GetComponent<RelationshipComponent>();
    auto& child_rel = child.GetComponent<RelationshipComponent>();

    // Set the child's parent to this entity
    child_rel.Parent = m_EntityHandle;

    // Insert the child into the linked list of siblings
    if (parent_rel.FirstChild != entt::null) {
        entt::entity current = parent_rel.FirstChild;

        // Traverse to the last sibling
        while (m_Scene->m_Registry.get<RelationshipComponent>(current).NextSibling != entt::null) {
            current = m_Scene->m_Registry.get<RelationshipComponent>(current).NextSibling;
        }

        // Append the new child at the end
        m_Scene->m_Registry.get<RelationshipComponent>(current).NextSibling = child;
        child_rel.PrevSibling = current;
    }
    else {
        // This is the first child
        parent_rel.FirstChild = child;
    }
}

Entity Entity::GetParent() {
    if (HasComponent<RelationshipComponent>()) {
        entt::entity parentHandle = GetComponent<RelationshipComponent>().Parent;
        if (parentHandle != entt::null) {
            return Entity(parentHandle, m_Scene);
        }
    }
    // Return an invalid entity if there is no parent
    return Entity(entt::null, m_Scene);
}

bool Entity::HasParent() {
    if (HasComponent<RelationshipComponent>()) {
        return GetComponent<RelationshipComponent>().Parent != entt::null;
    }
    return false;
}