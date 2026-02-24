#include "Texture.hpp"

Texture::Texture(const VulkanDevice& device, const std::string& filePath)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

	VulkanBuffer stagingBuffer(device, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    stagingBuffer.upload(pixels, imageSize);
	stbi_image_free(pixels);

    m_Image.emplace(device, texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);

    auto commandBuffer = device.beginSingleTimeCommands();
    device.transitionImageLayout(commandBuffer, m_Image->getImage(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eTransfer, {}, vk::AccessFlagBits2::eTransferWrite, vk::ImageAspectFlagBits::eColor);
    device.endSingleTimeCommands(commandBuffer);

    device.copyBufferToImage(*stagingBuffer.getBuffer(), *m_Image->getImage(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    auto commandBuffer2 = device.beginSingleTimeCommands();
    device.transitionImageLayout(commandBuffer2, m_Image->getImage(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eTransferWrite, vk::AccessFlagBits2::eShaderRead, vk::ImageAspectFlagBits::eColor);
    device.endSingleTimeCommands(commandBuffer2);

    m_Sampler = device.getDefaultSampler();
}

Texture::~Texture()
{
}
