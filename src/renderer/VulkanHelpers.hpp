#pragma once

// A simple bundle to hold our deduced synchronization masks
struct VulkanSyncInfo {
    vk::PipelineStageFlags2 stageMask;
    vk::AccessFlags2 accessMask;
};

// ==============================================================================
// DEDUCE SYNCHRONIZATION MASKS FROM LAYOUT
// ==============================================================================
inline VulkanSyncInfo GetSyncInfoFromLayout(vk::ImageLayout layout) {
    switch (layout) {
        // Initial state or "Don't Care" state. 
        // Operations usually start at the top of the pipe and don't wait on anything.
        case vk::ImageLayout::eUndefined:
            return { vk::PipelineStageFlagBits2::eTopOfPipe, vk::AccessFlagBits2::eNone };
            
        // Used when drawing to a color render target (like the Viewport or Swapchain)
        case vk::ImageLayout::eColorAttachmentOptimal:
            return { 
                vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
                vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead 
            };
            
        // Used when drawing to a depth buffer
        case vk::ImageLayout::eDepthAttachmentOptimal:
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return { 
                // Depth tests happen before (Early) and after (Late) the fragment shader runs
                vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, 
                vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead 
            };
                     
        // Used when a shader is reading a texture (like the ImGui pass reading the Viewport)
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return { 
                // We use AllGraphics here instead of just FragmentShader to be safe, 
                // in case you ever sample a texture inside your Vertex Shader!
                vk::PipelineStageFlagBits2::eAllGraphics, 
                vk::AccessFlagBits2::eShaderRead 
            };
            
        // Used right before handing the image to the OS windowing system to display
        case vk::ImageLayout::ePresentSrcKHR:
            return { 
                vk::PipelineStageFlagBits2::eBottomOfPipe, 
                vk::AccessFlagBits2::eNone 
            };
            
        // Used when copying data TO an image (e.g., uploading a texture from the CPU)
        case vk::ImageLayout::eTransferDstOptimal:
            return { 
                vk::PipelineStageFlagBits2::eTransfer, 
                vk::AccessFlagBits2::eTransferWrite 
            };

        // Used when copying data FROM an image (e.g., reading a pixel value back to the CPU)
        case vk::ImageLayout::eTransferSrcOptimal:
            return { 
                vk::PipelineStageFlagBits2::eTransfer, 
                vk::AccessFlagBits2::eTransferRead 
            };
            
        default:
            throw std::runtime_error("Unsupported layout transition encountered in Render Graph!");
    }
}


// ==============================================================================
// DEDUCE IMAGE ASPECT (COLOR VS DEPTH) FROM FORMAT
// ==============================================================================
inline vk::ImageAspectFlags GetAspectFlagsFromFormat(vk::Format format) {
    switch (format) {
        // Pure Depth formats
        case vk::Format::eD32Sfloat:
        case vk::Format::eD16Unorm:
            return vk::ImageAspectFlagBits::eDepth;
            
        // Combined Depth / Stencil formats
        case vk::Format::eD32SfloatS8Uint:
        case vk::Format::eD24UnormS8Uint:
        case vk::Format::eD16UnormS8Uint:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            
        // Everything else is assumed to be a standard Color format (RGBA, BGRA, etc.)
        default:
            return vk::ImageAspectFlagBits::eColor;
    }
}