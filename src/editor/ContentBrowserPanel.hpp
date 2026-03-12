#pragma once

#include "scene/AssetManager.hpp"
#include "renderer/VulkanRenderer.hpp"
#include <filesystem>

class ContentBrowserPanel
{
public:
    ContentBrowserPanel();

    // Nimmt wie gewohnt deine Manager an
    void OnImGuiRender(AssetManager& assetManager, VulkanRenderer& renderer);

private:
    void RefreshDirectory();

private:
    std::filesystem::path m_BaseDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::vector<std::filesystem::directory_entry> m_DirectoryEntries;
};