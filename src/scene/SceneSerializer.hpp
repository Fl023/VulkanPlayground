#pragma once

#include <yaml-cpp/yaml.h>
#include "core/UUID.hpp"

class Entity;
class Scene;
class AssetManager;
class VulkanRenderer;

class SceneSerializer {
public:
    SceneSerializer(Scene* scene, AssetManager* assetManager);

    void Serialize(const std::string& filepath);
    std::string SerializeToString();
    void SerializeRuntime(const std::string& filepath);

    bool Deserialize(const std::string& filepath, VulkanRenderer& renderer);
    bool DeserializeFromString(const std::string& yamlString, VulkanRenderer& renderer);
    bool DeserializeRuntime(const std::string& filepath, VulkanRenderer& renderer);

private:
    UUID GetUUIDFromEntity(Entity entity);
    bool IsChildOfModelInstance(Entity entity);
	void SerializeEntity(YAML::Emitter& out, Entity entity);
	void SerializeAssetRegistry(YAML::Emitter& out);


private:
    Scene* m_Scene;
    AssetManager* m_AssetManager;
};