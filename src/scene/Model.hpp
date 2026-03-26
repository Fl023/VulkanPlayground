#pragma once

#include "renderer/VulkanRenderer.hpp"
#include "Scene.hpp"
#include "Entity.hpp"
#include "renderer/VulkanVertex.hpp"
#include <tiny_gltf.h>

class AssetManager;

struct ModelNode {
    UUID ID;
    std::string Name;
    glm::mat4 LocalTransform{ 1.0f };

    AssetHandle MeshHandle = INVALID_ASSET_HANDLE;
    AssetHandle MaterialHandle = INVALID_ASSET_HANDLE;

    int32_t Parent = -1;
    int32_t FirstChild = -1;
    int32_t NextSibling = -1;
};

class Model {
public:
    Model(VulkanRenderer& renderer, AssetManager& assetManager, const std::string& filepath);

    // Creates actual EnTT entities from the stored blueprint
    Entity Instantiate(Scene* scene, AssetHandle modelHandle, Entity parent = {});

    const std::vector<ModelNode>& GetNodes() const { return m_Nodes; }
    const std::string& GetName() const { return m_Name; }
    const std::string& GetFilePath() const { return m_Filepath; }

private:
    int32_t ProcessNode(const tinygltf::Model& model, int gltf_node_index, int32_t parent_index, const std::vector<AssetHandle>& materialHandles);
    std::vector<AssetHandle> ExtractMaterials(const tinygltf::Model& model, const std::string& filepath);
    void ExtractPrimitiveGeometry(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<Vertex>& out_vertices, std::vector<uint16_t>& out_indices);

private:
    std::string m_Name;
    std::string m_Filepath;
	VulkanRenderer& m_Renderer;
	AssetManager& m_AssetManager;
    std::vector<ModelNode> m_Nodes;
    std::vector<int32_t> m_RootNodes; // Keep track of the top-level nodes
};