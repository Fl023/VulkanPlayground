#include "Scene.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "core/Input.hpp"
#include "scripts/ScriptEngine.hpp"

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(m_Registry.create(), this);

	entity.AddComponent<IDComponent>();
    entity.AddComponent<TransformComponent>();
    entity.AddComponent<RelationshipComponent>();
    auto& tag = entity.AddComponent<TagComponent>();
    tag.Tag = name.empty() ? "Entity" : name;

    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    if (!m_Registry.valid(entity)) return;

    if (entity.HasComponent<RelationshipComponent>()) {
        auto& rel = entity.GetComponent<RelationshipComponent>();

        // 1. Recursively destroy all children
        entt::entity currentChild = rel.FirstChild;
        while (currentChild != entt::null) {
            entt::entity nextSibling = m_Registry.get<RelationshipComponent>(currentChild).NextSibling;

            // Call DestroyEntity recursively
            DestroyEntity(Entity(currentChild, this));
            currentChild = nextSibling;
        }

        // 2. Remove this entity from its parent's linked list to prevent dangling pointers
        if (rel.Parent != entt::null) {
            auto& parentRel = m_Registry.get<RelationshipComponent>(rel.Parent);
            if (parentRel.FirstChild == entity) {
                parentRel.FirstChild = rel.NextSibling;
            }
        }
        if (rel.PrevSibling != entt::null) {
            m_Registry.get<RelationshipComponent>(rel.PrevSibling).NextSibling = rel.NextSibling;
        }
        if (rel.NextSibling != entt::null) {
            m_Registry.get<RelationshipComponent>(rel.NextSibling).PrevSibling = rel.PrevSibling;
        }
    }

    // 3. Finally, destroy the entity itself
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

void Scene::OnStart()
{
    m_Registry.view<NativeScriptComponent>().each([this](auto entityID, auto& nsc)
    {
        if (!nsc.Instance)
        {
            // 1. Call the Lambda Factory to allocate the memory!
            nsc.Instance = nsc.InstantiateScript();

            // 2. Hook up the ECS entity reference
            nsc.Instance->m_Entity = Entity{ entityID, this };

            // 3. Trigger the user's startup code
            nsc.Instance->OnCreate();
        }
    });

    sol::state& lua = ScriptEngine::GetState();

    // ==========================================
    // INITIALIZE LUA SCRIPTS
    // ==========================================
    m_Registry.view<LuaScriptComponent>().each([this](auto entityID, auto& lsc)
    {
        Entity entity{ entityID, this };

        if (ScriptEngine::LoadScript(lsc, entity))
        {
            if (lsc.OnCreate.valid()) {
                lsc.OnCreate();
            }
        }
    });
}

void Scene::OnUpdate(float deltaTime)
{
    auto physicsView = m_Registry.view<RigidBodyComponent, TransformComponent>();
    for (auto entityID : physicsView)
    {
        auto& rb = physicsView.get<RigidBodyComponent>(entityID);
        auto& transform = physicsView.get<TransformComponent>(entityID);

        // Apply Gravity Force
        if (rb.UseGravity) {
            // Standard Earth gravity is roughly 9.81 m/s^2 downwards
            rb.Acceleration.y = -9.81f;
        }

        // Integrate Velocity (Velocity = Velocity + Acceleration * Time)
        rb.Velocity += rb.Acceleration * deltaTime;

        // Integrate Position (Position = Position + Velocity * Time)
        transform.Translation += rb.Velocity * deltaTime;

        // Clear acceleration for the next frame (so forces don't infinitely stack)
        rb.Acceleration = glm::vec3(0.0f);
    }

    // ==========================================
    // TICK LUA SCRIPTS
    // ==========================================
    m_Registry.view<LuaScriptComponent>().each([=](auto entityID, auto& lsc)
    {
        if (lsc.OnUpdate.valid()) {
            // Using protected_function prevents the C++ engine from crashing if your Lua code has a typo!
            auto result = lsc.OnUpdate(deltaTime);
            if (!result.valid()) {
                sol::error err = result;
                std::cout << "Lua Update Error: " << err.what() << std::endl;
            }
        }
    });




    auto relView = m_Registry.view<TransformComponent, RelationshipComponent>();

    for (auto entityHandle : relView) {
        const auto& rel = relView.get<RelationshipComponent>(entityHandle);

        // We only want to start the recursion from ROOT nodes (nodes with no parent).
        // The recursive function will naturally reach all the children.
        if (rel.Parent == entt::null) {
            UpdateTransformHierarchy(entityHandle, glm::mat4(1.0f)); // Root nodes have an Identity parent matrix
        }
    }

    m_Registry.view<NativeScriptComponent>().each([=](auto entityID, auto& nsc)
    {
        if (nsc.Instance) {
            nsc.Instance->OnUpdate(deltaTime);
        }
    });
}

void Scene::OnUpdateEditor(float deltaTime)
{
    // ==========================================
    // UPDATE TRANSFORMS (Required for Gizmos and rendering!)
    // ==========================================
    auto relView = m_Registry.view<TransformComponent, RelationshipComponent>();

    for (auto entityHandle : relView) {
        const auto& rel = relView.get<RelationshipComponent>(entityHandle);

        // We only want to start the recursion from ROOT nodes
        if (rel.Parent == entt::null) {
            UpdateTransformHierarchy(entityHandle, glm::mat4(1.0f));
        }
    }

    // NOTE: If you later add an "Editor Camera" that flies around when you hold Right-Click,
    // you would put its Update() function right here!
}

void Scene::OnStop()
{
    m_Registry.view<NativeScriptComponent>().each([=](auto entityID, auto& nsc)
    {
        if (nsc.Instance) {
            nsc.Instance->OnDestroy();
            nsc.DestroyScript(&nsc);
        }
    });

    m_Registry.view<LuaScriptComponent>().each([=](auto entityID, auto& lsc)
    {
        if (lsc.OnDestroy.valid()) {
            auto result = lsc.OnDestroy();
            if (!result.valid()) {
                sol::error err = result;
                std::cout << "Lua OnDestroy Error: " << err.what() << std::endl;
            }
        }
	});
}

void Scene::OnEvent(Event& event)
{
    // We want to send events to scripts before the C++ engine processes them, so that Lua scripts have a chance to "consume" the event and prevent it from reaching the C++ code.
    m_Registry.view<LuaScriptComponent>().each([&event](auto entityID, auto& lsc)
    {
        if (lsc.OnEvent.valid()) {
            auto result = lsc.OnEvent(event);
            if (!result.valid()) {
                sol::error err = result;
                std::cout << "Lua OnEvent Error: " << err.what() << std::endl;
            }
        }
    });

    m_Registry.view<NativeScriptComponent>().each([&event](auto entityID, auto& nsc)
    {
        if (nsc.Instance) {
            nsc.Instance->OnEvent(event);
        }
	});
}

void Scene::OnViewportResize(uint32_t width, uint32_t height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;

    // Update all cameras in the scene that don't have a fixed aspect ratio
    auto view = m_Registry.view<CameraComponent>();
    for (auto entity : view)
    {
        auto& cameraComponent = view.get<CameraComponent>(entity);
        if (!cameraComponent.FixedAspectRatio)
        {
            cameraComponent.SceneCamera.SetViewportSize(width, height);
        }
    }
}

void Scene::Clear() {
    m_Registry.clear();
}

void Scene::SetPrimaryCamera(Entity cameraEntity)
{
    // 1. Validate the entity actually has a camera
    if (!cameraEntity.HasComponent<CameraComponent>()) return;

    // 2. Loop through all cameras and turn them off
    auto view = m_Registry.view<CameraComponent>();
    for (auto entityID : view)
    {
        if (entityID != (entt::entity)cameraEntity)
        {
            view.get<CameraComponent>(entityID).Primary = false;
        }
    }

    // 3. Turn the target camera on
    cameraEntity.GetComponent<CameraComponent>().Primary = true;
}

Entity Scene::GetPrimaryCameraEntity()
{
    auto view = m_Registry.view<CameraComponent>();
    for (auto entityID : view)
    {
        if (view.get<CameraComponent>(entityID).Primary)
        {
            return Entity{ entityID, this };
        }
    }
    return {}; // Return a null entity if none exists
}

