#pragma once

#include "scene/Scene.hpp"
#include "scene/Entity.hpp"
#include "scene/AssetManager.hpp"
#include "renderer/VulkanRenderer.hpp"

class EditorUI
{
public:
    EditorUI() = default;
    ~EditorUI() = default;

    // Diese Methode wird jeden Frame aufgerufen
    void Draw(Scene& scene, AssetManager& assetManager, VulkanRenderer& renderer);

private:
    // Die Auswahl-Variable zieht aus der main.cpp hierher um!
    Entity m_SelectedEntity; 
};