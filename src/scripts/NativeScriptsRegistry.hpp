#pragma once

#include "scene/Entity.hpp"
#include "scene/Components.hpp"

// A factory function that takes an entity and binds a specific script to it
using ScriptFactoryFunction = std::function<void(Entity)>;

class NativeScriptRegistry 
{
public:
    // Call this in your Application startup to register all your game scripts
    template<typename T>
    static void Register(const std::string& name)
    {
        s_Registry[name] = [name](Entity entity) {
            // Safely get or add the component before binding the C++ class
            if (!entity.HasComponent<NativeScriptComponent>()) {
                entity.AddComponent<NativeScriptComponent>().Bind<T>(name);
            }
            else {
                entity.GetComponent<NativeScriptComponent>().Bind<T>(name);
            }
        };
    }

    // Call this during Deserialization to attach the script
    static bool BindScript(Entity entity, const std::string& name)
    {
        if (s_Registry.find(name) != s_Registry.end()) {
            s_Registry[name](entity); // Execute the lambda to bind the component
            return true;
        }
        return false;
    }

    static const std::unordered_map<std::string, ScriptFactoryFunction>& GetRegistry() { return s_Registry; }

private:
    static std::unordered_map<std::string, ScriptFactoryFunction> s_Registry;
};