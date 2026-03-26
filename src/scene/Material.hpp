#pragma once
#include "AssetHandle.hpp"

class Material {
public:
    Material(const std::string& name, AssetHandle albedoHandle = INVALID_ASSET_HANDLE)
        : m_Name(name), m_AlbedoHandle(albedoHandle) {
    }

    const std::string& GetName() const { return m_Name; }

    AssetHandle GetTextureHandle() const { return m_AlbedoHandle; }
    void SetTextureHandle(AssetHandle handle) { m_AlbedoHandle = handle; }

private:
    std::string m_Name;
    AssetHandle m_AlbedoHandle = INVALID_ASSET_HANDLE;
};