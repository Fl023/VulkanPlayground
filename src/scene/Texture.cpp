#include "Texture.hpp"

Texture::Texture(const VulkanDevice& device, const std::string& filePath)
	: m_device(device), m_FilePath(filePath)
{
    int texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &m_texWidth, &m_texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = m_texWidth * m_texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_texWidth, m_texHeight)))) + 1;
	createVulkanImage(pixels, imageSize);
	stbi_image_free(pixels);
}

Texture::Texture(const VulkanDevice& device, uint32_t width, uint32_t height, const void* pixels)
    : m_device(device), m_FilePath(""), m_texWidth(width), m_texHeight(height), m_mipLevels(1)
{
    vk::DeviceSize imageSize = width * height * 4;
    createVulkanImage(pixels, imageSize);
}

Texture::~Texture()
{
}

void Texture::createVulkanImage(const void* pixels, vk::DeviceSize imageSize)
{
    VulkanBuffer stagingBuffer(m_device, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    stagingBuffer.upload(pixels, imageSize);

    m_Image.emplace(m_device, m_texWidth, m_texHeight, m_mipLevels, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);

    auto commandBuffer = m_device.beginSingleTimeCommands();
    m_device.transitionImageLayout(commandBuffer, m_Image->getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, m_mipLevels, vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eTransfer, {}, vk::AccessFlagBits2::eTransferWrite, vk::ImageAspectFlagBits::eColor);
    m_device.endSingleTimeCommands(commandBuffer);

    m_device.copyBufferToImage(*stagingBuffer.getBuffer(), *m_Image->getImage(), static_cast<uint32_t>(m_texWidth), static_cast<uint32_t>(m_texHeight));

    generateMipmaps();

    m_Sampler = m_device.getDefaultSampler();
}

void Texture::generateMipmaps()
{
    // Check if image format supports linear blit-ing
    vk::FormatProperties formatProperties = m_device.getPhysicalDevice().getFormatProperties(m_Image->getFormat());

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    auto commandBuffer = m_device.beginSingleTimeCommands();

    vk::ImageMemoryBarrier2 barrier{
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite, 
        .dstAccessMask = vk::AccessFlagBits2::eTransferRead, 
        .oldLayout = vk::ImageLayout::eTransferDstOptimal, 
        .newLayout = vk::ImageLayout::eTransferSrcOptimal, 
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored, 
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = m_Image->getImage(),
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    // Die Hülle für pipelineBarrier2
    vk::DependencyInfo dependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    int32_t mipWidth = m_texWidth;
    int32_t mipHeight = m_texHeight;

    for (uint32_t i = 1; i < m_mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;

        // 1. Das vorherige Level: TRANSFER_DST -> TRANSFER_SRC
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;

        commandBuffer.pipelineBarrier2(dependencyInfo);

        // 2. Der moderne Blit-Befehl (blitImage2)
        vk::ImageBlit2 blit{
            .srcSubresource = { vk::ImageAspectFlagBits::eColor, i - 1, 0, 1 },
            .srcOffsets = {{ vk::Offset3D(0, 0, 0), vk::Offset3D(mipWidth, mipHeight, 1) }},
            .dstSubresource = { vk::ImageAspectFlagBits::eColor, i, 0, 1 },
            .dstOffsets = {{ vk::Offset3D(0, 0, 0), vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1) }}
        };

        vk::BlitImageInfo2 blitInfo{
            .srcImage = m_Image->getImage(),
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = m_Image->getImage(),
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &blit,
            .filter = vk::Filter::eLinear
        };

        commandBuffer.blitImage2(blitInfo);

        // 3. Das vorherige Level ist fertig: TRANSFER_SRC -> SHADER_READ
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        commandBuffer.pipelineBarrier2(dependencyInfo);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // 4. Das allerletzte Mip-Level manuell in den Shader-Read Modus bringen
    barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    commandBuffer.pipelineBarrier2(dependencyInfo);

    m_device.endSingleTimeCommands(commandBuffer);
}
