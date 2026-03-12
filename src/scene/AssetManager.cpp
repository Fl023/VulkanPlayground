#include "AssetManager.hpp"
#include "Texture.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "renderer/VulkanRenderer.hpp"


AssetManager::~AssetManager() { Clear(); }

AssetHandle AssetManager::LoadOrCreateTexture(VulkanRenderer& renderer, const std::string& name, const std::string& filepath)
{
    // 1. Check if the name already exists
    if (m_TextureRegistry.find(name) != m_TextureRegistry.end()) {
        std::cerr << "Warning: Texture name '" << name << "' already exists! Returning existing handle.\n";
        return m_TextureRegistry[name];
    }

    // 2. Deduplication: Check if this exact filepath is already loaded
    for (const auto& [existingHandle, existingTexture] : m_Textures) {
        if (!existingTexture->GetFilePath().empty() && existingTexture->GetFilePath() == filepath) {
            m_TextureRegistry[name] = existingHandle; // Create an alias
            return existingHandle;
        }
    }

    // 3. Completely new texture: Generate Handle, Allocate, and Register
    AssetHandle handle = GenerateHandle();

    // Allocate on the heap, but give temporary ownership to a local unique_ptr
    auto texture = std::make_unique<Texture>(renderer.getDevice(), filepath);

    // Register it with the Vulkan Bindless Array (Pass the raw pointer!)
    renderer.AddTextureToBindlessArray(texture.get());

    // Move the unique_ptr into the Vault
    m_Textures[handle] = std::move(texture);

    // Link the name to the Handle in the Registry
    m_TextureRegistry[name] = handle;

    return handle;
}

AssetHandle AssetManager::CreateMaterial(const std::string& name, AssetHandle albedoHandle)
{
    if (m_MaterialRegistry.find(name) != m_MaterialRegistry.end()) {
        return m_MaterialRegistry[name];
    }

    AssetHandle handle = GenerateHandle();
    m_Materials[handle] = std::make_unique<Material>(name, albedoHandle);
    m_MaterialRegistry[name] = handle;
    return handle;
}

AssetHandle AssetManager::AddMesh(const std::string& name, std::unique_ptr<Mesh> mesh)
{
    if (m_MeshRegistry.find(name) != m_MeshRegistry.end()) {
        return m_MeshRegistry[name];
    }

    AssetHandle handle = GenerateHandle();
    m_Meshes[handle] = std::move(mesh);
    m_MeshRegistry[name] = handle;
    return handle;
}

Texture* AssetManager::GetTexture(AssetHandle handle) const
{
    auto it = m_Textures.find(handle);
    if (it != m_Textures.end()) {
        return it->second.get(); // Safe raw pointer to the vault's memory
    }
    return nullptr;
}

Texture* AssetManager::GetTexture(const std::string& name) const
{
    auto it = m_TextureRegistry.find(name);
    if (it != m_TextureRegistry.end()) {
        return GetTexture(it->second);
    }
    return nullptr;
}

Material* AssetManager::GetMaterial(AssetHandle handle) const
{
    auto it = m_Materials.find(handle);
    return it != m_Materials.end() ? it->second.get() : nullptr;
}

Material* AssetManager::GetMaterial(const std::string& name) const
{
    auto it = m_MaterialRegistry.find(name);
    return it != m_MaterialRegistry.end() ? GetMaterial(it->second) : nullptr;
}

Mesh* AssetManager::GetMesh(AssetHandle handle) const
{
    auto it = m_Meshes.find(handle);
    return it != m_Meshes.end() ? it->second.get() : nullptr;
}

Mesh* AssetManager::GetMesh(const std::string& name) const
{
    auto it = m_MeshRegistry.find(name);
    return it != m_MeshRegistry.end() ? GetMesh(it->second) : nullptr;
}

AssetHandle AssetManager::GetTextureHandle(const std::string& name) const {
    auto it = m_TextureRegistry.find(name);
    return it != m_TextureRegistry.end() ? it->second : INVALID_ASSET_HANDLE;
}

AssetHandle AssetManager::GetMaterialHandle(const std::string& name) const
{
    auto it = m_MaterialRegistry.find(name);
	return it != m_MaterialRegistry.end() ? it->second : INVALID_ASSET_HANDLE;
}

AssetHandle AssetManager::GetMeshHandle(const std::string& name) const
{
    auto it = m_MeshRegistry.find(name);
    return it != m_MeshRegistry.end() ? it->second : INVALID_ASSET_HANDLE;
}

void AssetManager::RemoveTexture(const std::string& name)
{
    auto it = m_TextureRegistry.find(name);
    if (it != m_TextureRegistry.end()) {
        AssetHandle handleToDelete = it->second;

        // 1. Destroy the Vulkan memory by erasing the unique_ptr from the Vault
        m_Textures.erase(handleToDelete);

        // 2. Erase every name alias in the registry that pointed to this handle
        for (auto regIt = m_TextureRegistry.begin(); regIt != m_TextureRegistry.end(); ) {
            if (regIt->second == handleToDelete) {
                regIt = m_TextureRegistry.erase(regIt);
            }
            else {
                ++regIt;
            }
        }
    }
}

void AssetManager::RemoveMaterial(const std::string& name)
{
    auto it = m_MaterialRegistry.find(name);
    if (it != m_MaterialRegistry.end()) {
        m_Materials.erase(it->second);
        m_MaterialRegistry.erase(it);
    }
}

void AssetManager::RemoveMesh(const std::string& name)
{
    auto it = m_MeshRegistry.find(name);
    if (it != m_MeshRegistry.end()) {
        m_Meshes.erase(it->second);
        m_MeshRegistry.erase(it);
	}
}

void AssetManager::Clear()
{
    m_TextureRegistry.clear();
    m_MaterialRegistry.clear();
    m_MeshRegistry.clear();

    m_Textures.clear();
    m_Materials.clear();
    m_Meshes.clear();
}


AssetHandle AssetManager::GenerateHandle()
{
    static std::random_device rd;
    static std::mt19937_64 eng(rd());
    static std::uniform_int_distribution<uint64_t> dist;

    AssetHandle handle = dist(eng);
    while (handle == INVALID_ASSET_HANDLE) {
        handle = dist(eng); // Make sure we never generate 0
    }
    return handle;
}
