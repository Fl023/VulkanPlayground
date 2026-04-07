#include "NativeScriptsRegistry.hpp"

// Define the static map
std::unordered_map<std::string, ScriptFactoryFunction> NativeScriptRegistry::s_Registry;