#pragma once

#include "RGCommandList.hpp"
#include "scene/RenderView.hpp"
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
#include <functional>

// --- VIRTUAL RESOURCES ---
using RGTextureHandle = uint32_t;
constexpr RGTextureHandle RG_INVALID_TEXTURE = 0xFFFFFFFF;

struct RGTextureDesc {
    std::string Name;
    uint32_t Width;
    uint32_t Height;
    vk::Format Format;
};

struct RGResourceUsage {
    RGTextureHandle Handle;
    vk::ImageLayout Layout;
    enum class Access { Read, Write } Type;
};

struct RGTransition {
    RGTextureHandle Handle;
    vk::ImageLayout OldLayout;
    vk::ImageLayout NewLayout;
};

struct RGPhysicalResource {
    vk::Image Image;
    vk::ImageView ImageView;
};

// --- THE BUILDER ---
class RenderGraph; // Forward declaration
struct RenderPassNode;

class RGBuilder {
public:
    RGBuilder(RenderGraph& graph, RenderPassNode& currentPass)
        : m_Graph(graph), m_Pass(currentPass) {}

    RGTextureHandle CreateTexture(const std::string& name, uint32_t width, uint32_t height, vk::Format format);
    void Read(RGTextureHandle handle, vk::ImageLayout requiredLayout);
    void Write(RGTextureHandle handle, vk::ImageLayout requiredLayout);

private:
    RenderGraph& m_Graph;
    RenderPassNode& m_Pass;
};

// --- THE PASS NODE ---
struct RenderPassNode {
    std::string Name;
    std::vector<RGResourceUsage> Inputs;
    std::vector<RGResourceUsage> Outputs;

    std::function<void(RGBuilder&)> Setup;
    std::function<void(RGCommandList&, const RenderView&)> Execute;

    std::vector<RGTransition> RequiredTransitions;
};

// --- THE RENDER GRAPH ---
class RenderGraph {
public:
    RenderGraph() = default;

    void AddPass(const std::string& name, 
                 std::function<void(RGBuilder&)> setup, 
                 std::function<void(RGCommandList&, const RenderView&)> execute);

    void Compile(); // Analyzes Inputs/Outputs and generates barriers
    void Clear();   // Resets the graph for the next frame

    RGTextureHandle RegisterTexture(const RGTextureDesc& desc);

    const std::vector<RenderPassNode>& GetPasses() const { return m_Passes; }
    const std::vector<RGTextureDesc>& GetVirtualTextures() const { return m_VirtualTextures; }

    RGTextureHandle ImportTexture(const std::string& name, vk::Format format, vk::Image image, vk::ImageView imageView) {
        RGTextureDesc desc{ name, 0, 0, format };
        RGTextureHandle handle = RegisterTexture(desc);
        m_PhysicalImages[handle] = { image, imageView };
        return handle;
    }

    RGPhysicalResource* GetPhysicalImage(RGTextureHandle handle) {
        if (m_PhysicalImages.find(handle) != m_PhysicalImages.end()) {
            return &m_PhysicalImages.at(handle);
        }
        return nullptr;
    }

private:
    std::vector<RenderPassNode> m_Passes;
    std::vector<RGTextureDesc> m_VirtualTextures;

    std::unordered_map<RGTextureHandle, RGPhysicalResource> m_PhysicalImages;
};