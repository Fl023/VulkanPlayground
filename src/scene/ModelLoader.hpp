#pragma once

#include "renderer/VulkanRenderer.hpp"
#include "Scene.hpp"
#include "Entity.hpp"
#include "renderer/VulkanVertex.hpp"
#include <tiny_gltf.h>

class AssetManager;

class ModelLoader {
public:
    // Parses the glTF file, creates entities in the scene, 
    // and registers meshes with the AssetManager.
    // Returns the root entity of the loaded model.
    static Entity LoadGltf(Scene* scene, VulkanRenderer& renderer, AssetManager& assetManager, const std::string& filepath);

private:
    static void ProcessNode(Scene* scene, VulkanRenderer& renderer, AssetManager& assetManager, const tinygltf::Model& model, int node_index, Entity parent_entity, const std::vector<AssetHandle>& materialHandles);
    static std::vector<AssetHandle> ExtractMaterials(const tinygltf::Model& model, const std::string& filepath, VulkanRenderer& renderer, AssetManager& assetManager);
    static void ExtractPrimitiveGeometry(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<Vertex>& out_vertices, std::vector<uint16_t>& out_indices);
};