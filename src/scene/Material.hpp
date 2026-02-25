#pragma once

#include "Texture.hpp"

class Material {
public:
    Material(std::shared_ptr<Texture> albedo, vk::raii::DescriptorSet set)
        : albedoTexture(std::move(albedo)), descriptorSet(std::move(set)) {
    }

    const vk::raii::DescriptorSet& getDescriptorSet() const { return descriptorSet; }
    std::shared_ptr<Texture> getTexture() const { return albedoTexture; }

private:
    std::shared_ptr<Texture> albedoTexture;
    vk::raii::DescriptorSet descriptorSet;
};