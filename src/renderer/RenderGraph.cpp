#include "RenderGraph.hpp"

// --- RGBuilder Implementation ---
RGTextureHandle RGBuilder::CreateTexture(const std::string& name, uint32_t width, uint32_t height, vk::Format format) {
    RGTextureDesc desc{ name, width, height, format };
    return m_Graph.RegisterTexture(desc);
}

void RGBuilder::Read(RGTextureHandle handle, vk::ImageLayout requiredLayout) {
    m_Pass.Inputs.push_back({ handle, requiredLayout, RGResourceUsage::Access::Read });
}

void RGBuilder::Write(RGTextureHandle handle, vk::ImageLayout requiredLayout) {
    m_Pass.Outputs.push_back({ handle, requiredLayout, RGResourceUsage::Access::Write });
}

// --- RenderGraph Implementation ---
void RenderGraph::AddPass(const std::string& name, 
                          std::function<void(RGBuilder&)> setup, 
                          std::function<void(RGCommandList&, const RenderView&)> execute) 
{
    RenderPassNode& pass = m_Passes.emplace_back();
    pass.Name = name;
    pass.Setup = std::move(setup);
    pass.Execute = std::move(execute);

    // Run setup immediately to populate Inputs/Outputs
    RGBuilder builder(*this, pass);
    if (pass.Setup) {
        pass.Setup(builder);
    }
}

RGTextureHandle RenderGraph::RegisterTexture(const RGTextureDesc& desc) {
    RGTextureHandle newHandle = static_cast<RGTextureHandle>(m_VirtualTextures.size());
    m_VirtualTextures.push_back(desc);
    return newHandle;
}

void RenderGraph::Compile() {
    // 1. The Global State Tracker
    // This maps our Virtual Texture ID -> Its current layout in the timeline
    std::unordered_map<RGTextureHandle, vk::ImageLayout> textureStates;

    // 2. Simulate the frame
    for (RenderPassNode& pass : m_Passes) {

        // Clear old transitions if we are recompiling
        pass.RequiredTransitions.clear();

        // 3. Process Inputs (Reads)
        for (const RGResourceUsage& input : pass.Inputs) {
            vk::ImageLayout currentLayout = vk::ImageLayout::eUndefined;
            if (textureStates.contains(input.Handle)) {
                currentLayout = textureStates[input.Handle];
            }

            // YOUR LOGIC: If they differ, transition!
            if (currentLayout != input.Layout) {
                pass.RequiredTransitions.push_back({
                    input.Handle, currentLayout, input.Layout
                    });
                // Update the global tracker to the new state
                textureStates[input.Handle] = input.Layout;
            }
        }

        // 4. Process Outputs (Writes)
        for (const RGResourceUsage& output : pass.Outputs) {
            vk::ImageLayout currentLayout = vk::ImageLayout::eUndefined;
            if (textureStates.contains(output.Handle)) {
                currentLayout = textureStates[output.Handle];
            }

            // YOUR LOGIC: If they differ, transition!
            if (currentLayout != output.Layout) {
                pass.RequiredTransitions.push_back({
                    output.Handle, currentLayout, output.Layout
                    });
                // Update the global tracker to the new state
                textureStates[output.Handle] = output.Layout;
            }
        }
    }
}

void RenderGraph::Clear() {
    m_Passes.clear();
    m_VirtualTextures.clear();
}