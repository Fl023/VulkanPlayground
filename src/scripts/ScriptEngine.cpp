#include "ScriptEngine.hpp"
#include "scene/Entity.hpp"
#include "scene/Components.hpp"
#include "core/Input.hpp"

sol::state ScriptEngine::s_LuaState;

void ScriptEngine::Init()
{
    // 1. Open standard Lua libraries (math, string, etc.)
    s_LuaState.open_libraries(sol::lib::base, sol::lib::math);

    // 2. Bind C++ Math (GLM)
    s_LuaState.new_usertype<glm::vec3>("vec3",
        sol::constructors<glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z
    );

    // 3. Bind your Components!
    s_LuaState.new_usertype<TransformComponent>("TransformComponent",
        "Translation", &TransformComponent::Translation,
        // Since Rotation is hidden behind setters, bind the getter/setter!
        "GetEulerAngles", &TransformComponent::GetEulerAngles,
        "SetEulerAngles", &TransformComponent::SetEulerAngles
    );

    // 4. Bind the Entity class so Lua can call entity:GetComponent()
    s_LuaState.new_usertype<Entity>("Entity",
        "HasTransform", &Entity::HasComponent<TransformComponent>,
        "GetTransform", [](Entity& e) -> TransformComponent& { return e.GetComponent<TransformComponent>(); }
        // Note: sol2 requires lambda wrappers for templates like GetComponent<T>()
    );

    // 5. Bind Input!
    s_LuaState.new_enum("KeyCode",
        "W", KeyCode::W,
        "A", KeyCode::A,
        "S", KeyCode::S,
        "D", KeyCode::D,
        "Space", KeyCode::Space
    );
    s_LuaState.set_function("IsKeyPressed", &Input::IsKeyPressed);
}

void ScriptEngine::Shutdown() {}
sol::state& ScriptEngine::GetState() { return s_LuaState; }

bool ScriptEngine::LoadScript(LuaScriptComponent& component, Entity entity)
{
    if (component.ScriptPath.empty()) return false;

    sol::state& lua = GetState();

    // 1. Create fresh environment
    component.Environment = sol::environment(lua, sol::create, lua.globals());

    // 2. Inject the entity
    component.Environment["entity"] = entity;

    // 3. Load the file
    auto loadResult = lua.script_file(component.ScriptPath, component.Environment);
    if (loadResult.valid())
    {
        // 4. Bind functions
        component.OnCreate = component.Environment["OnCreate"];
        component.OnUpdate = component.Environment["OnUpdate"];
        component.OnDestroy = component.Environment["OnDestroy"];
		component.OnEvent = component.Environment["OnEvent"];
        return true;
    }
    else
    {
        sol::error err = loadResult;
        std::cerr << "Failed to load script: " << err.what() << std::endl;

        // Invalidate the environment on failure
        component.Environment = sol::environment();
        return false;
    }
}
