#pragma once

#include "AssetHandle.hpp"

class Texture;
class Material;
class Mesh;
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

    AssetHandle CreateMaterial(const std::string& name, AssetHandle albedoHandle);

    AssetHandle AddMesh(const std::string& name, std::unique_ptr<Mesh> mesh);

    Texture* GetTexture(AssetHandle handle) const;

    Texture* GetTexture(const std::string& name) const;

    Material* GetMaterial(AssetHandle handle) const;

    Material* GetMaterial(const std::string& name) const;

    Mesh* GetMesh(AssetHandle handle) const;

    Mesh* GetMesh(const std::string& name) const;

    AssetHandle GetTextureHandle(const std::string& name) const;
	AssetHandle GetMaterialHandle(const std::string& name) const;
	AssetHandle GetMeshHandle(const std::string& name) const;

    const std::unordered_map<std::string, AssetHandle>& GetTextureRegistry() const { return m_TextureRegistry; }
    const std::unordered_map<std::string, AssetHandle>& GetMaterialRegistry() const { return m_MaterialRegistry; }
    const std::unordered_map<std::string, AssetHandle>& GetMeshRegistry() const { return m_MeshRegistry; }


    void RemoveTexture(VulkanRenderer& renderer, const std::string& name);
    void RemoveMaterial(const std::string& name);
	void RemoveMesh(const std::string& name);
    void Clear();

private:
    // Generates a robust, guaranteed non-zero UUID
    AssetHandle GenerateHandle();

private:
    // --- THE VAULTS (Actual Memory Ownership) ---
    std::unordered_map<AssetHandle, std::unique_ptr<Texture>> m_Textures;
    std::unordered_map<AssetHandle, std::unique_ptr<Material>> m_Materials;
    std::unordered_map<AssetHandle, std::unique_ptr<Mesh>> m_Meshes;

    // --- THE REGISTRIES (String to Handle mapping) ---
    std::unordered_map<std::string, AssetHandle> m_TextureRegistry;
    std::unordered_map<std::string, AssetHandle> m_MaterialRegistry;
    std::unordered_map<std::string, AssetHandle> m_MeshRegistry;
};