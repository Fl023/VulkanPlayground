#pragma once

#include "Texture.hpp"

class Material {
public:
    Material(const std::string& name, std::shared_ptr<Texture> albedo, vk::raii::DescriptorSet set)
        : m_Name(name), albedoTexture(std::move(albedo)), descriptorSet(std::move(set)) {
    }

    const std::string& GetName() const { return m_Name; }
    const vk::raii::DescriptorSet& getDescriptorSet() const { return descriptorSet; }
    std::shared_ptr<Texture> getTexture() const { return albedoTexture; }

    void SetTexture(std::shared_ptr<Texture> texture) { albedoTexture = std::move(texture); }

private:
    std::string m_Name;
    std::shared_ptr<Texture> albedoTexture;
    vk::raii::DescriptorSet descriptorSet;
};