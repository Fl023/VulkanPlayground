#pragma once

#include "AssetHandle.hpp"

class Texture;
class Material;
class Mesh;
class Model;
class VulkanRenderer;


class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager();

    // ==========================================
    // CREATION & LOADING
    // ==========================================

    AssetHandle LoadOrCreateTexture(VulkanRenderer& renderer, const std::string& name, const std::string& filepath);
    AssetHandle LoadOrCreateTextureWithHandle(AssetHandle handle, VulkanRenderer& renderer, const std::string& name, const std::string& filepath);

    AssetHandle LoadCubemap(VulkanRenderer& renderer, const std::string& name, const std::array<std::string, 6>& facePaths);
	AssetHandle LoadCubemapWithHandle(AssetHandle handle, VulkanRenderer& renderer, const std::string& name, const std::array<std::string, 6>& facePaths);

    AssetHandle CreateMaterial(const std::string& name, AssetHandle albedoHandle);
    AssetHandle CreateMaterialWithHandle(AssetHandle handle, const std::string& name, AssetHandle albedoHandle);

    AssetHandle AddMesh(const std::string& name, std::unique_ptr<Mesh> mesh);
    AssetHandle AddMeshWithHandle(AssetHandle handle, const std::string& name, std::unique_ptr<Mesh> mesh);

    AssetHandle LoadModel(VulkanRenderer& renderer, const std::string& name, const std::string& filepath);
    AssetHandle LoadModelWithHandle(AssetHandle handle, VulkanRenderer& renderer, const std::string& name, const std::string& filepath);

    Texture* GetTexture(AssetHandle handle) const;
    Texture* GetTexture(const std::string& name) const;

    Material* GetMaterial(AssetHandle handle) const;
    Material* GetMaterial(const std::string& name) const;

    Mesh* GetMesh(AssetHandle handle) const;
    Mesh* GetMesh(const std::string& name) const;

    Model* GetModel(AssetHandle handle) const;
    Model* GetModel(const std::string& name) const;

    AssetHandle GetTextureHandle(const std::string& name) const;
	AssetHandle GetMaterialHandle(const std::string& name) const;
	AssetHandle GetMeshHandle(const std::string& name) const;
	AssetHandle GetModelHandle(const std::string& name) const;

    std::string GetTextureName(AssetHandle handle) const;
    std::string GetMaterialName(AssetHandle handle) const;
    std::string GetMeshName(AssetHandle handle) const;
    std::string GetModelName(AssetHandle handle) const;

    const std::unordered_map<std::string, AssetHandle>& GetTextureRegistry() const { return m_TextureRegistry; }
    const std::unordered_map<std::string, AssetHandle>& GetMaterialRegistry() const { return m_MaterialRegistry; }
    const std::unordered_map<std::string, AssetHandle>& GetMeshRegistry() const { return m_MeshRegistry; }
    const std::unordered_map<std::string, AssetHandle>& GetModelRegistry() const { return m_ModelRegistry; }


    void RemoveTexture(VulkanRenderer& renderer, const std::string& name);
    void RemoveMaterial(const std::string& name);
	void RemoveMesh(const std::string& name);
    void Clear();

private:
    // --- THE VAULTS (Actual Memory Ownership) ---
    std::unordered_map<AssetHandle, std::unique_ptr<Texture>> m_Textures;
    std::unordered_map<AssetHandle, std::unique_ptr<Material>> m_Materials;
    std::unordered_map<AssetHandle, std::unique_ptr<Mesh>> m_Meshes;
    std::unordered_map<AssetHandle, std::unique_ptr<Model>> m_Models;

    // --- THE REGISTRIES (String to Handle mapping) ---
    std::unordered_map<std::string, AssetHandle> m_TextureRegistry;
    std::unordered_map<std::string, AssetHandle> m_MaterialRegistry;
    std::unordered_map<std::string, AssetHandle> m_MeshRegistry;
    std::unordered_map<std::string, AssetHandle> m_ModelRegistry;
};