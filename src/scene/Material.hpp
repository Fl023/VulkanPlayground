#pragma once
#include "AssetHandle.hpp"

class VulkanPipeline;

enum class CullMode { Back, Front, None };
enum class BlendMode { Opaque, AlphaBlend, Additive };
enum class DepthState { ReadWrite, ReadOnly, Off };

// The blueprint for how a material behaves
struct MaterialRenderState {
    std::string ShaderPath = "shaders/shader.spv";
    CullMode Culling = CullMode::Back;
    BlendMode Blending = BlendMode::Opaque;
    DepthState Depth = DepthState::ReadWrite;
    bool IsWireframe = false;

    // A quick helper to generate a unique string key for the pipeline cache
    std::string GenerateHash(uint32_t targetFormat) const {
        return ShaderPath + "_" +
            std::to_string((int)Culling) + "_" +
            std::to_string((int)Blending) + "_" +
            std::to_string((int)Depth) + "_" +
            std::to_string(IsWireframe) + "_" +
            std::to_string(targetFormat);
    }
};

class Material {
public:
    Material(const std::string& name, const MaterialRenderState& state, VulkanPipeline* pipeline, AssetHandle albedoHandle = INVALID_ASSET_HANDLE)
        : m_Name(name), m_State(state), m_Pipeline(pipeline), m_AlbedoHandle(albedoHandle) {
    }

    const std::string& GetName() const { return m_Name; }

    VulkanPipeline* GetPipeline() const { return m_Pipeline; }

    AssetHandle GetTextureHandle() const { return m_AlbedoHandle; }
    void SetTextureHandle(AssetHandle handle) { m_AlbedoHandle = handle; }

    const MaterialRenderState& GetState() const { return m_State; }

private:
    std::string m_Name;
    MaterialRenderState m_State;
    VulkanPipeline* m_Pipeline = nullptr;
    AssetHandle m_AlbedoHandle = INVALID_ASSET_HANDLE;
};