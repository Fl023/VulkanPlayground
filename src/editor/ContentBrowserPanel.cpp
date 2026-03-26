#include "ContentBrowserPanel.hpp"
#include "FileDialogs.hpp" // Für deinen Datei-Dialog
#include <imgui.h>
#include <imgui_stdlib.h>

ContentBrowserPanel::ContentBrowserPanel()
{
    m_BaseDirectory = std::filesystem::current_path();
    m_CurrentDirectory = m_BaseDirectory;

    RefreshDirectory();
}

void ContentBrowserPanel::RefreshDirectory()
{
    m_DirectoryEntries.clear();
    for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
    {
        m_DirectoryEntries.push_back(directoryEntry);
    }
}

void ContentBrowserPanel::OnImGuiRender(AssetManager& assetManager, VulkanRenderer& renderer)
{
    ImGui::Begin("Content Browser");

    if (m_CurrentDirectory != m_BaseDirectory)
    {
        if (ImGui::Button("<- Back"))
        {
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
            RefreshDirectory();
        }
        ImGui::Separator();
    }

    static float padding = 16.0f;
    static float thumbnailSize = 96.0f;
    float cellSize = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    for (auto& directoryEntry : m_DirectoryEntries)
    {
        const auto& path = directoryEntry.path();
        std::string filenameString = path.filename().generic_string();

        ImGui::PushID(filenameString.c_str());

        if (ImGui::Button(directoryEntry.is_directory() ? "[DIR]" : "[FILE]", { thumbnailSize, thumbnailSize }))
        {
            // Klick-Logik (als Alternative zum DoubleClick)
        }

        if (ImGui::BeginDragDropSource())
        {
            std::filesystem::path relativePath(path);
            const wchar_t* itemPath = relativePath.c_str();
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (directoryEntry.is_directory())
                m_CurrentDirectory /= path.filename();
                RefreshDirectory();
        }

        ImGui::TextWrapped("%s", filenameString.c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    ImGui::End();


    ImGui::Begin("Loaded VRAM Assets");

    // --- TEXTURES ---
    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
    {
        std::string textureToDelete = "";
        // Iterate the Registry instead of the Vault!
        for (const auto& [name, handle] : assetManager.GetTextureRegistry()) {
            ImGui::PushID(name.c_str());
            ImGui::Text("%s", name.c_str());
            ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f);
            if (ImGui::Button("X")) textureToDelete = name;
            ImGui::PopID();
        }

        if (!textureToDelete.empty()) {
            renderer.getDevice().getDevice().waitIdle(); // Keep this safe stop for now!

            // Free the bindless index before destroying the memory
            Texture* tex = assetManager.GetTexture(textureToDelete);
            if (tex) renderer.FreeBindlessIndex(tex->GetBindlessIndex());

            assetManager.RemoveTexture(renderer, textureToDelete);
        }

        ImGui::Spacing();
        static std::string newTextureName = "";
        static std::string newTexturePath = "";
        ImGui::InputText("Name##Tex", &newTextureName);
        ImGui::InputText("Path##Tex", &newTexturePath);
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const wchar_t* path = (const wchar_t*)payload->Data;
                std::filesystem::path texturePath(path);

                if (texturePath.extension() == ".png" || texturePath.extension() == ".jpg")
                {
                    newTextureName = texturePath.stem().generic_string();
                    newTexturePath = texturePath.generic_string();
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            std::string filepath = FileDialogs::OpenFile("Image Files (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0All Files (*.*)\0*.*\0");
            if (!filepath.empty()) {
                newTexturePath = filepath;
                size_t lastSlash = filepath.find_last_of("/\\");
                size_t lastDot = filepath.find_last_of(".");
                if (lastSlash != std::string::npos && lastDot != std::string::npos && lastDot > lastSlash)
                    newTextureName = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
                else newTextureName = "NewTexture";
            }
        }

        if (ImGui::Button("Load Texture")) {
            if (!newTexturePath.empty() && !newTextureName.empty()) {
                assetManager.LoadOrCreateTexture(renderer, newTextureName, newTexturePath);
                newTexturePath = ""; newTextureName = "";
            }
        }
    }

    ImGui::Separator();

    // --- CUBEMAPS ---
    if (ImGui::CollapsingHeader("Cubemaps", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextDisabled("Cubemaps share memory with Textures and can be deleted there.");
        ImGui::Spacing();

        static std::string newCubemapName = "NewSkybox";
        ImGui::InputText("Cubemap Name", &newCubemapName);
        ImGui::Spacing();

        static std::array<std::string, 6> facePaths = { "", "", "", "", "", "" };
        const char* faceLabels[6] = {
            "Right (+X)", "Left (-X)", "Top (+Y)",
            "Bottom (-Y)", "Front (+Z)", "Back (-Z)"
        };

        bool allFacesSelected = true;

        for (int i = 0; i < 6; i++) {
            ImGui::PushID(i);

            // Text input for the path
            ImGui::InputText(faceLabels[i], &facePaths[i]);

            // Allow drag and drop from the Content Browser directly into the text box!
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                    const wchar_t* path = (const wchar_t*)payload->Data;
                    std::filesystem::path texturePath(path);

                    if (texturePath.extension() == ".png" || texturePath.extension() == ".jpg") {
                        facePaths[i] = texturePath.generic_string();
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine();
            if (ImGui::Button("Browse...")) {
                std::string filepath = FileDialogs::OpenFile("Image Files (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0All Files (*.*)\0*.*\0");
                if (!filepath.empty()) {
                    facePaths[i] = filepath;
                }
            }
            ImGui::PopID();

            // Check if any slot is still empty
            if (facePaths[i].empty()) {
                allFacesSelected = false;
            }
        }

        ImGui::Spacing();

        // Disable the load button unless all 6 faces and a name are provided
        ImGui::BeginDisabled(!allFacesSelected || newCubemapName.empty());
        if (ImGui::Button("Load Cubemap", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {

            assetManager.LoadCubemap(renderer, newCubemapName, facePaths);

            // Reset the form after successful load
            newCubemapName = "NewSkybox";
            for (auto& path : facePaths) {
                path = "";
            }
        }
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    // --- MATERIALS ---
    if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
    {
        std::string materialToDelete = "";
        for (const auto& [name, handle] : assetManager.GetMaterialRegistry()) {
            ImGui::PushID(name.c_str());
            bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f);
            if (ImGui::Button("X")) materialToDelete = name;

            if (open) {
                Material* material = assetManager.GetMaterial(handle);
                if (material) {
                    std::string albedoName = "Select Texture...";
                    for (const auto& [texName, texHandle] : assetManager.GetTextureRegistry()) {
                        if (material->GetTextureHandle() == texHandle) { albedoName = texName; break; }
                    }

                    if (ImGui::BeginCombo("Albedo Texture", albedoName.c_str())) {
                        for (const auto& [texName, texHandle] : assetManager.GetTextureRegistry()) {
                            bool isSelected = (material->GetTextureHandle() == texHandle);

                            // Instant CPU Swap! Just pass the handle integer.
                            if (ImGui::Selectable(texName.c_str(), isSelected)) material->SetTextureHandle(texHandle);

                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        if (!materialToDelete.empty()) {
            assetManager.RemoveMaterial(materialToDelete);
        }

        ImGui::Spacing();
        static std::string newMatName = "NewMaterial";
        ImGui::InputText("Name##Mat", &newMatName);

        // Use an AssetHandle for the selection instead of a pointer!
        static AssetHandle selectedAlbedo = INVALID_ASSET_HANDLE;

        std::string previewName = "Select Texture...";
        for (const auto& [texName, texHandle] : assetManager.GetTextureRegistry()) {
            if (selectedAlbedo == texHandle) { previewName = texName; break; }
        }

        if (ImGui::BeginCombo("Albedo Texture##NewMat", previewName.c_str())) {
            for (const auto& [name, handle] : assetManager.GetTextureRegistry()) {
                bool isSelected = (selectedAlbedo == handle);
                if (ImGui::Selectable(name.c_str(), isSelected)) selectedAlbedo = handle;
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Create Material")) {
            if (!newMatName.empty()) {
                assetManager.CreateMaterial(newMatName, selectedAlbedo);
                selectedAlbedo = INVALID_ASSET_HANDLE;
                newMatName = "NewMaterial";
            }
        }
    }
    ImGui::End();
}