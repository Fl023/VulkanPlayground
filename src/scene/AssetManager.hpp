#pragma once

#include "Texture.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

class AssetManager
{
public:
    AssetManager() = default;

    ~AssetManager() { Clear(); }

    std::shared_ptr<Texture> GetTexture(const std::string& name) const
    {
        auto it = m_Textures.find(name);
        if (it != m_Textures.end()) {
            return it->second;
        }
        return nullptr; // (Oder du gibst hier deine weiße Default-Textur zurück!)
    }

    std::shared_ptr<Texture> LoadTexture(const VulkanDevice& device, const std::string& name, const std::string& filepath)
    {
        // Warnung, falls der Name schon existiert (Optional, aber gut für's Debugging)
        if (m_Textures.find(name) != m_Textures.end()) {
            std::cerr << "Warning: Texture name '" << name << "' already exists! Returning existing texture.\n";
            return m_Textures[name];
        }

        // Deduplizierung: Prüfen, ob GENAU DIESE DATEI schon im VRAM ist
        for (const auto& [existingName, existingTexture] : m_Textures) {
            // Wir prüfen nur Texturen, die auch wirklich einen Pfad haben
            if (!existingTexture->GetFilePath().empty() && existingTexture->GetFilePath() == filepath) {
                m_Textures[name] = existingTexture; // Alias unter neuem Namen anlegen
                return existingTexture;
            }
        }

        // Komplett neu von der Festplatte laden
        auto texture = std::make_shared<Texture>(device, filepath);
        m_Textures[name] = texture;
        return texture;
    }

    std::string GetTextureName(std::shared_ptr<Texture> texture) const
    {
        if (!texture) return "None";

        for (const auto& [name, tex] : m_Textures) {
            if (tex == texture) {
                return name;
            }
        }
        return "Unknown";
    }

    void AddTexture(const std::string& name, std::shared_ptr<Texture> texture)
    {
        m_Textures[name] = texture;
    }

    void AddMaterial(const std::string& name, std::shared_ptr<Material> material)
    {
        m_Materials[name] = material;
    }

    void AddMesh(const std::string& name, std::shared_ptr<Mesh> mesh)
    {
        m_Meshes[name] = mesh;
    }

    void RemoveTexture(const std::string& name) {
        m_Textures.erase(name);
    }

    void RemoveMaterial(const std::string& name) {
        m_Materials.erase(name);
    }

    const std::unordered_map<std::string, std::shared_ptr<Texture>>& GetTextures() const { return m_Textures; }
    const std::unordered_map<std::string, std::shared_ptr<Material>>& GetMaterials() const { return m_Materials; }
    const std::unordered_map<std::string, std::shared_ptr<Mesh>>& GetMeshes() const { return m_Meshes; }

    void Clear()
    {
        m_Materials.clear();
        m_Meshes.clear();
        m_Textures.clear();
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Meshes;
};