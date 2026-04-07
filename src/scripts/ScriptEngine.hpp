#pragma once
#include <sol/sol.hpp>

class Entity;
struct LuaScriptComponent;

class ScriptEngine 
{
public:
    static void Init();
    static void Shutdown();
    static sol::state& GetState();

    static bool LoadScript(LuaScriptComponent& component, Entity entity);
private:
    static sol::state s_LuaState;
};