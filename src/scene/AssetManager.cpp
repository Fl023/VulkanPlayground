#include "AssetManager.hpp"
#include "Texture.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "renderer/VulkanRenderer.hpp"
#include "renderer/VulkanPipeline.hpp"

AssetManager::~AssetManager() { Clear(); }

AssetHandle AssetManager::LoadOrCreateTexture(VulkanRenderer& renderer, const std::string& name, const std::string& filepath)
{
    // 1. DEDUPLICATION: Is this exact file already in VRAM?
    for (const auto& [existingHandle, existingTexture] : m_Textures) {
        // Only check 2D textures (which have filepaths, unlike Cubemaps which might not return a single string)
        if (!existingTexture->GetFilePath().empty() && existingTexture->GetFilePath() == filepath) {

            if (m_TextureRegistry.find(name) != m_TextureRegistry.end() && m_TextureRegistry[name] != existingHandle) {
                std::cerr << "Warning: You tried to alias texture filepath '" << filepath
                    << "' to '" << name << "', but that name belongs to a different asset! Alias ignored.\n";
            }
            else {
                m_TextureRegistry[name] = existingHandle;
            }
            return existingHandle;
        }
    }

    // 2. NAME COLLISION: Auto-rename loop
    std::string safeName = name;
    int suffix = 1;
    while (m_TextureRegistry.find(safeName) != m_TextureRegistry.end()) {
        safeName = name + "_" + std::to_string(suffix);
        suffix++;
    }

    if (safeName != name) {
        std::cout << "Notice: Texture name collision. '" << name << "' auto-renamed to '" << safeName << "'\n";
    }

    // 3. LOAD & REGISTER
    AssetHandle handle;
    auto texture = std::make_unique<Texture>(renderer.getDevice(), filepath);
    renderer.AddTextureToBindlessArray(texture.get());
    m_Textures[handle] = std::move(texture);
    m_TextureRegistry[safeName] = handle;

    return handle;
}

AssetHandle AssetManager::LoadOrCreateTextureWithHandle(AssetHandle handle, VulkanRenderer& renderer, const std::string& name, const std::string& filepath)
{
    // 1. ALIAS REGISTRATION
    std::string safeName = name;
    bool needsAlias = true;

    if (m_TextureRegistry.find(name) != m_TextureRegistry.end()) {
        if (m_TextureRegistry[name] == handle) {
            // This exact name already points to this exact handle. We're good!
            needsAlias = false;
        }
        else {
            // Collision! The name is taken by a DIFFERENT asset. Auto-rename.
            int suffix = 1;
            while (m_TextureRegistry.find(safeName) != m_TextureRegistry.end()) {
                safeName = name + "_" + std::to_string(suffix);
                suffix++;
            }
            std::cout << "Notice: Deserialization alias collision. '" << name << "' renamed to '" << safeName << "'\n";
        }
    }

    if (needsAlias) {
        m_TextureRegistry[safeName] = handle;
    }

    // 2. MEMORY ALLOCATION (Only if the vault doesn't have it yet!)
    if (m_Textures.find(handle) == m_Textures.end()) {
        auto texture = std::make_unique<Texture>(renderer.getDevice(), filepath);
        renderer.AddTextureToBindlessArray(texture.get());
        m_Textures[handle] = std::move(texture);
    }

    return handle;
}

AssetHandle AssetManager::LoadCubemap(VulkanRenderer& renderer, const std::string& name, const std::array<std::string, 6>& facePaths)
{
    // 1. NAME COLLISION: Auto-rename loop
    // Prevents accidentally overwriting an existing skybox or 2D texture alias
    std::string safeName = name;
    int suffix = 1;

    // Note: It shares the same registry as normal Textures!
    while (m_TextureRegistry.find(safeName) != m_TextureRegistry.end()) {
        safeName = name + "_" + std::to_string(suffix);
        suffix++;
    }

    if (safeName != name) {
        std::cout << "Notice: Cubemap name collision. '" << name
            << "' auto-renamed to '" << safeName << "'\n";
    }

    // 2. LOAD & REGISTER: Generate Handle, Allocate, and Map safely
    AssetHandle handle;

    // Use the Cubemap constructor
    auto texture = std::make_unique<Texture>(renderer.getDevice(), facePaths);

    renderer.AddTextureToBindlessArray(texture.get());

    m_Textures[handle] = std::move(texture);
    m_TextureRegistry[safeName] = handle;

    return handle;
}

AssetHandle AssetManager::LoadCubemapWithHandle(AssetHandle handle, VulkanRenderer& renderer, const std::string& name, const std::array<std::string, 6>& facePaths)
{
    // 1. ALIAS REGISTRATION
    std::string safeName = name;
    bool needsAlias = true;
    if (m_TextureRegistry.find(name) != m_TextureRegistry.end()) {
        if (m_TextureRegistry[name] == handle) {
            needsAlias = false;
        }
        else {
            int suffix = 1;
            while (m_TextureRegistry.find(safeName) != m_TextureRegistry.end()) {
                safeName = name + "_" + std::to_string(suffix);
                suffix++;
            }
        }
    }
    if (needsAlias) {
        m_TextureRegistry[safeName] = handle;
    }
    // 2. MEMORY ALLOCATION
    if (m_Textures.find(handle) == m_Textures.end()) {
        auto texture = std::make_unique<Texture>(renderer.getDevice(), facePaths);
        renderer.AddTextureToBindlessArray(texture.get());
        m_Textures[handle] = std::move(texture);
    }

    return handle;
}

AssetHandle AssetManager::CreateMaterial(VulkanRenderer& renderer, const std::string& name, const MaterialRenderState& state, vk::Format targetFormat, AssetHandle albedoHandle)
{
    // NAME COLLISION: Auto-rename loop
    std::string safeName = name;
    int suffix = 1;
    while (m_MaterialRegistry.find(safeName) != m_MaterialRegistry.end()) {
        safeName = name + "_" + std::to_string(suffix);
        suffix++;
    }

    // 1. Resolve the Pipeline!
    VulkanPipeline* pipeline = GetOrCreatePipeline(renderer, state, targetFormat);

    // 2. Create the Material with the resolved pipeline
    AssetHandle handle;
    m_Materials[handle] = std::make_unique<Material>(safeName, state, pipeline, albedoHandle);
    m_MaterialRegistry[safeName] = handle;
    return handle;
}

AssetHandle AssetManager::CreateMaterialWithHandle(AssetHandle handle, VulkanRenderer& renderer, const std::string& name, const MaterialRenderState& state, vk::Format targetFormat, AssetHandle albedoHandle)
{
    // 1. ALIAS REGISTRATION
    std::string safeName = name;
    bool needsAlias = true;

    if (m_MaterialRegistry.find(name) != m_MaterialRegistry.end()) {
        if (m_MaterialRegistry[name] == handle) {
            needsAlias = false;
        }
        else {
            int suffix = 1;
            while (m_MaterialRegistry.find(safeName) != m_MaterialRegistry.end()) {
                safeName = name + "_" + std::to_string(suffix);
                suffix++;
            }
        }
    }

    if (needsAlias) {
        m_MaterialRegistry[safeName] = handle;
    }

    VulkanPipeline* pipeline = GetOrCreatePipeline(renderer, state, targetFormat);

    // 2. MEMORY ALLOCATION
    if (m_Materials.find(handle) == m_Materials.end()) {
        m_Materials[handle] = std::make_unique<Material>(safeName, state, pipeline, albedoHandle);
    }

    return handle;
}

AssetHandle AssetManager::AddMesh(const std::string& name, std::unique_ptr<Mesh> mesh)
{
    // NAME COLLISION: Auto-rename loop
    std::string safeName = name;
    int suffix = 1;
    while (m_MeshRegistry.find(safeName) != m_MeshRegistry.end()) {
        safeName = name + "_" + std::to_string(suffix);
        suffix++;
    }

    AssetHandle handle;
    m_Meshes[handle] = std::move(mesh);
    m_MeshRegistry[safeName] = handle;
    return handle;
}

AssetHandle AssetManager::AddMeshWithHandle(AssetHandle handle, const std::string& name, std::unique_ptr<Mesh> mesh)
{
    if (m_Meshes.find(handle) != m_Meshes.end()) {
        return handle;
    }

    // NAME COLLISION: Auto-rename loop
    std::string safeName = name;
    int suffix = 1;
    while (m_MeshRegistry.find(safeName) != m_MeshRegistry.end()) {
        safeName = name + "_" + std::to_string(suffix);
        suffix++;
    }

    m_Meshes[handle] = std::move(mesh);
    m_MeshRegistry[safeName] = handle;

    return handle;
}

Texture* AssetManager::GetTexture(AssetHandle handle) const
{
    auto it = m_Textures.find(handle);
    if (it != m_Textures.end()) {
        return it->second.get(); // Safe raw pointer to the vault's memory
    }
    return nullptr;
}

Texture* AssetManager::GetTexture(const std::string& name) const
{
    auto it = m_TextureRegistry.find(name);
    if (it != m_TextureRegistry.end()) {
        return GetTexture(it->second);
    }
    return nullptr;
}

Material* AssetManager::GetMaterial(AssetHandle handle) const
{
    auto it = m_Materials.find(handle);
    return it != m_Materials.end() ? it->second.get() : nullptr;
}

Material* AssetManager::GetMaterial(const std::string& name) const
{
    auto it = m_MaterialRegistry.find(name);
    return it != m_MaterialRegistry.end() ? GetMaterial(it->second) : nullptr;
}

Mesh* AssetManager::GetMesh(AssetHandle handle) const
{
    auto it = m_Meshes.find(handle);
    return it != m_Meshes.end() ? it->second.get() : nullptr;
}

Mesh* AssetManager::GetMesh(const std::string& name) const
{
    auto it = m_MeshRegistry.find(name);
    return it != m_MeshRegistry.end() ? GetMesh(it->second) : nullptr;
}

AssetHandle AssetManager::GetTextureHandle(const std::string& name) const {
    auto it = m_TextureRegistry.find(name);
    return it != m_TextureRegistry.end() ? it->second : INVALID_ASSET_HANDLE;
}

AssetHandle AssetManager::GetMaterialHandle(const std::string& name) const
{
    auto it = m_MaterialRegistry.find(name);
	return it != m_MaterialRegistry.end() ? it->second : INVALID_ASSET_HANDLE;
}

AssetHandle AssetManager::GetMeshHandle(const std::string& name) const
{
    auto it = m_MeshRegistry.find(name);
    return it != m_MeshRegistry.end() ? it->second : INVALID_ASSET_HANDLE;
}

AssetHandle AssetManager::GetModelHandle(const std::string& name) const
{
    auto it = m_ModelRegistry.find(name);
    return it != m_ModelRegistry.end() ? it->second : INVALID_ASSET_HANDLE;
}

std::string AssetManager::GetTextureName(AssetHandle handle) const {
    for (const auto& [registryName, mappedHandle] : m_TextureRegistry) {
        if (mappedHandle == handle) {
            return registryName;
        }
    }
    return "Unknown Texture";
}

std::string AssetManager::GetMaterialName(AssetHandle handle) const {
    for (const auto& [registryName, mappedHandle] : m_MaterialRegistry) {
        if (mappedHandle == handle) {
            return registryName;
        }
    }
    return "Unknown Material";
}

std::string AssetManager::GetMeshName(AssetHandle handle) const {
    for (const auto& [registryName, mappedHandle] : m_MeshRegistry) {
        if (mappedHandle == handle) {
            return registryName;
        }
    }
    return "Unknown Mesh";
}

std::string AssetManager::GetModelName(AssetHandle handle) const {
    for (const auto& [registryName, mappedHandle] : m_ModelRegistry) {
        if (mappedHandle == handle) {
            return registryName;
        }
    }
    return "Unknown Model";
}

AssetHandle AssetManager::LoadModel(VulkanRenderer& renderer, const std::string& name, const std::string& filepath) {
    // 1. DEDUPLICATION: Is this exact file already in VRAM?
    for (const auto& [handle, model] : m_Models) {
        if (model->GetFilePath() == filepath) {
            // The file is already loaded! 
            // Check if they are trying to hijack an existing, different name:
            if (m_ModelRegistry.find(name) != m_ModelRegistry.end() && m_ModelRegistry[name] != handle) {
                std::cerr << "Warning: You tried to alias filepath '" << filepath
                    << "' to '" << name << "', but that name belongs to a different asset! Alias ignored.\n";
            }
            else {
                // Safe to add this new alias pointing to the existing file
                m_ModelRegistry[name] = handle;
            }
            return handle;
        }
    }

    // 2. NAME COLLISION: The file is new, but is the requested name already taken?
    std::string safeName = name;
    int suffix = 1;

    // Auto-rename loop (e.g., "Tree" -> "Tree_1" -> "Tree_2")
    while (m_ModelRegistry.find(safeName) != m_ModelRegistry.end()) {
        safeName = name + "_" + std::to_string(suffix);
        suffix++;
    }

    if (safeName != name) {
        std::cout << "Notice: Asset name collision. '" << name
            << "' auto-renamed to '" << safeName << "'\n";
    }

    // 3. LOAD & REGISTER: We have a unique file and a guaranteed safe string key!
    AssetHandle handle;
    m_Models[handle] = std::make_unique<Model>(renderer, *this, filepath);
    m_ModelRegistry[safeName] = handle;

    return handle;
}

AssetHandle AssetManager::LoadModelWithHandle(AssetHandle handle, VulkanRenderer& renderer, const std::string& name, const std::string& filepath)
{
    // 1. ALIAS REGISTRATION
    std::string safeName = name;
    bool needsAlias = true;

    if (m_ModelRegistry.find(name) != m_ModelRegistry.end()) {
        if (m_ModelRegistry[name] == handle) {
            needsAlias = false;
        }
        else {
            int suffix = 1;
            while (m_ModelRegistry.find(safeName) != m_ModelRegistry.end()) {
                safeName = name + "_" + std::to_string(suffix);
                suffix++;
            }
        }
    }

    if (needsAlias) {
        m_ModelRegistry[safeName] = handle;
    }

    // 2. MEMORY ALLOCATION
    if (m_Models.find(handle) == m_Models.end()) {
        m_Models[handle] = std::make_unique<Model>(renderer, *this, filepath);
    }

    return handle;
}

Model* AssetManager::GetModel(AssetHandle handle) const
{
    auto it = m_Models.find(handle);
    return it != m_Models.end() ? it->second.get() : nullptr;
}

Model* AssetManager::GetModel(const std::string& name) const
{
    auto it = m_ModelRegistry.find(name);
    return it != m_ModelRegistry.end() ? GetModel(it->second) : nullptr;
}

void AssetManager::RemoveTexture(VulkanRenderer& renderer, const std::string& name)
{
    auto it = m_TextureRegistry.find(name);
    if (it != m_TextureRegistry.end()) {
        AssetHandle handleToDelete = it->second;

        // 1. EXTRACT the node from the map without destroying the unique_ptr
        auto textureNode = m_Textures.extract(handleToDelete);

        if (!textureNode.empty()) {
            // 2. Take ownership and convert to shared_ptr JUST for the queue!
            std::shared_ptr<Texture> texToKill = std::move(textureNode.mapped());
            uint32_t bindlessIndex = texToKill->GetBindlessIndex();

            // 3. Pass the shared_ptr into the lambda by value
            renderer.SubmitToDeletionQueue([tex = texToKill, bindlessIndex, &renderer]() {

                // Free the bindless slot for future textures
                renderer.FreeBindlessIndex(bindlessIndex, tex->IsCubemap());

                // The lambda dies, the shared_ptr drops to 0, and the texture is destroyed!
                });
        }

        // 4. Clean up the string registries exactly as you did before
        for (auto regIt = m_TextureRegistry.begin(); regIt != m_TextureRegistry.end(); ) {
            if (regIt->second == handleToDelete) regIt = m_TextureRegistry.erase(regIt);
            else ++regIt;
        }
    }
}

void AssetManager::RemoveMaterial(const std::string& name)
{
    auto it = m_MaterialRegistry.find(name);
    if (it != m_MaterialRegistry.end()) {
        m_Materials.erase(it->second);
        m_MaterialRegistry.erase(it);
    }
}

void AssetManager::RemoveMesh(const std::string& name)
{
    auto it = m_MeshRegistry.find(name);
    if (it != m_MeshRegistry.end()) {
        m_Meshes.erase(it->second);
        m_MeshRegistry.erase(it);
	}
}

void AssetManager::Clear()
{
    m_TextureRegistry.clear();
    m_MaterialRegistry.clear();
    m_MeshRegistry.clear();
	m_ModelRegistry.clear();

    m_Textures.clear();
    m_Materials.clear();
    m_Meshes.clear();
	m_Models.clear();
}

VulkanPipeline* AssetManager::GetOrCreatePipeline(VulkanRenderer& renderer, const MaterialRenderState& state, vk::Format targetFormat)
{
    // 1. Check if we already compiled this exact setup
    std::string hash = state.GenerateHash(static_cast<uint32_t>(targetFormat));
    if (m_PipelineCache.find(hash) != m_PipelineCache.end()) {
        return m_PipelineCache[hash].get();
    }

    const VulkanDevice& device = renderer.getDevice();

    // 2. Start with sensible defaults
    PipelineConfigInfo config{};
    VulkanPipeline::defaultPipelineConfigInfo(config, device.getMsaaSamples());

    // 3. APPLY OVERRIDES FROM THE FRONTEND STATE

    // Culling
    if (state.Culling == CullMode::None) {
        config.rasterizationInfo.cullMode = vk::CullModeFlagBits::eNone;
    }
    else if (state.Culling == CullMode::Front) {
        config.rasterizationInfo.cullMode = vk::CullModeFlagBits::eFront;
    }

    // Wireframe
    if (state.IsWireframe) {
        config.rasterizationInfo.polygonMode = vk::PolygonMode::eLine;
    }

    // Blending (Simple Alpha Blend Example)
    if (state.Blending == BlendMode::AlphaBlend) {
        config.colorBlendAttachment.blendEnable = vk::True;
        config.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        config.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        config.colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        config.colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        config.colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        config.colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    }

    // Depth
    if (state.Depth == DepthState::ReadOnly) {
        config.depthStencilInfo.depthWriteEnable = vk::False;
    }
    else if (state.Depth == DepthState::Off) {
        config.depthStencilInfo.depthTestEnable = vk::False;
        config.depthStencilInfo.depthWriteEnable = vk::False;
    }

    // 4. Set the dynamic rendering format
    config.colorAttachmentFormats = { targetFormat };
    config.depthAttachmentFormat = vk::Format::eD32Sfloat; // Or dynamically pass this in too!

    // 5. Load the unified shader module using your new Reflection system!
    VulkanShader shader(device, state.ShaderPath, renderer.GetGeneralPipelineSignature());

    // 6. Compile, store, and return
    auto newPipeline = std::make_unique<VulkanPipeline>(device, shader, config);
    VulkanPipeline* ptr = newPipeline.get();

    m_PipelineCache[hash] = std::move(newPipeline);

    return ptr;
}
