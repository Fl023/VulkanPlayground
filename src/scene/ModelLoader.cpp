#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "ModelLoader.hpp"
#include "Components.hpp"
#include "AssetManager.hpp"


// Helper to extract vertices and indices from a glTF primitive
void ModelLoader::ExtractPrimitiveGeometry(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<Vertex>& out_vertices, std::vector<uint16_t>& out_indices) {
    const float* positionData = nullptr;
    const float* texCoordData = nullptr;
    size_t vertexCount = 0;

    // --- EXTRACT VERTICES ---
    if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        positionData = reinterpret_cast<const float*>(&model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]);
        vertexCount = accessor.count;
    }

    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        texCoordData = reinterpret_cast<const float*>(&model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]);
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        Vertex vertex{};
        // Your Vertex struct uses 'pos', 'color', and 'texCoord'
        if (positionData) {
            vertex.pos = { positionData[i * 3 + 0], positionData[i * 3 + 1], positionData[i * 3 + 2] };
        }

        vertex.color = { 1.0f, 1.0f, 1.0f }; // Default to white since glTF doesn't always provide colors

        if (texCoordData) {
            vertex.texCoord = { texCoordData[i * 2 + 0], texCoordData[i * 2 + 1] };
        }

        out_vertices.push_back(vertex);
    }

    // --- EXTRACT INDICES ---
    if (primitive.indices >= 0) {
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

        const void* dataPtr = &(indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);

        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* indices = static_cast<const uint16_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                out_indices.push_back(indices[i]);
            }
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* indices = static_cast<const uint32_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                // Your Mesh class expects uint16_t, so we safely cast it down
                out_indices.push_back(static_cast<uint16_t>(indices[i]));
            }
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const uint8_t* indices = static_cast<const uint8_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                out_indices.push_back(static_cast<uint16_t>(indices[i]));
            }
        }
    }
}

// Recursively processes glTF nodes into your ECS scene hierarchy
void ModelLoader::ProcessNode(Scene* scene, VulkanRenderer& renderer, AssetManager& assetManager, const tinygltf::Model& model, int node_index, Entity parent_entity, const std::vector<AssetHandle>& materialHandles) {
    const auto& node = model.nodes[node_index];

    // 1. Create Node Entity
    std::string node_name = node.name.empty() ? "Node_" + std::to_string(node_index) : node.name;
    Entity node_entity = scene->CreateEntity(node_name);

    // 2. Extract Transform
    auto& transform = node_entity.GetComponent<TransformComponent>();

    if (node.matrix.size() == 16) {
        transform.SetLocalTransform(glm::make_mat4(node.matrix.data()));
    }
    else {
        if (node.translation.size() == 3) {
            transform.Translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
        }
        if (node.rotation.size() == 4) {
            transform.SetRotation(glm::make_quat(node.rotation.data()));
        }
        if (node.scale.size() == 3) {
            transform.Scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
        }
    }

    // 3. Attach to parent
    if (parent_entity) {
        parent_entity.AddChild(node_entity);
    }

    // 4. Process Meshes & Primitives
    if (node.mesh >= 0) {
        const auto& mesh = model.meshes[node.mesh];
        int primitive_index = 0;

        for (const auto& primitive : mesh.primitives) {
            std::string prim_name = node_name + "_Prim" + std::to_string(primitive_index++);
            Entity prim_entity = scene->CreateEntity(prim_name);

            // Attach primitive entity to the structural node entity
            node_entity.AddChild(prim_entity);

            // Extract the geometry using the helper function
            std::vector<Vertex> vertices;
            std::vector<uint16_t> indices;
            ExtractPrimitiveGeometry(model, primitive, vertices, indices);

            // Create Vulkan Mesh using unique_ptr for the AssetManager
            auto vulkanMesh = std::make_unique<Mesh>(renderer.getDevice(), prim_name, vertices, indices);

            // Register with AssetManager and get the AssetHandle
            AssetHandle meshHandle = assetManager.AddMesh(prim_name, std::move(vulkanMesh));

            // Assign handle to the ECS component
            prim_entity.AddComponent<MeshComponent>(meshHandle);

            if (primitive.material >= 0 && primitive.material < materialHandles.size()) {
                prim_entity.AddComponent<MaterialComponent>(materialHandles[primitive.material]);
            }
            else {
                // Fallback if the mesh has no material assigned in the glTF file
                prim_entity.AddComponent<MaterialComponent>(INVALID_ASSET_HANDLE);
            }
        }
    }

    // 5. Recurse for children
    for (int child_index : node.children) {
        ProcessNode(scene, renderer, assetManager, model, child_index, node_entity, materialHandles);
    }
}

std::vector<AssetHandle> ModelLoader::ExtractMaterials(const tinygltf::Model& model, const std::string& filepath, VulkanRenderer& renderer, AssetManager& assetManager)
{
    std::vector<AssetHandle> materialHandles;
    materialHandles.reserve(model.materials.size());
    std::filesystem::path basePath = std::filesystem::path(filepath).parent_path();
	std::cout << "Extracting materials from glTF model...\n";
	std::cout << "Base path for textures: " << basePath << std::endl;

    for (size_t i = 0; i < model.materials.size(); i++) {
        const auto& gltfMat = model.materials[i];
        std::string matName = gltfMat.name.empty() ? "Material_" + std::to_string(i) : gltfMat.name;

        AssetHandle albedoHandle = INVALID_ASSET_HANDLE;
        int targetTextureIndex = -1;

        // 1. Try standard Metallic-Roughness workflow first
        if (gltfMat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            targetTextureIndex = gltfMat.pbrMetallicRoughness.baseColorTexture.index;
        }
        // 2. Fallback to Specular-Glossiness extension if it exists
        else if (gltfMat.extensions.find("KHR_materials_pbrSpecularGlossiness") != gltfMat.extensions.end()) {
            const auto& ext = gltfMat.extensions.at("KHR_materials_pbrSpecularGlossiness");
            if (ext.Has("diffuseTexture")) {
                targetTextureIndex = ext.Get("diffuseTexture").Get("index").GetNumberAsInt();
            }
        }

        // If we found a valid texture index from either workflow, load it
        if (targetTextureIndex >= 0) {
            int imageIndex = model.textures[targetTextureIndex].source;
            const auto& image = model.images[imageIndex];

            if (!image.uri.empty()) {
                std::string texPath = (basePath / image.uri).string();
                albedoHandle = assetManager.LoadOrCreateTexture(renderer, image.uri, texPath);
            }
            else {
                std::cout << "Warning: Embedded textures require loading from memory!\n";
            }
        }

        // Create the material in the AssetManager and store the handle
        AssetHandle matHandle = assetManager.CreateMaterial(matName, albedoHandle);
        materialHandles.push_back(matHandle);
    }

    return materialHandles;
}

Entity ModelLoader::LoadGltf(Scene* scene, VulkanRenderer& renderer, AssetManager& assetManager, const std::string& filepath) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool success = false;
    if (filepath.substr(filepath.find_last_of(".") + 1) == "glb") {
        success = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
    } else {
        success = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
    }

    if (!warn.empty()) std::cout << "glTF warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "glTF error: " << err << std::endl;
    if (!success) throw std::runtime_error("Failed to load glTF model: " + err);

    std::vector<AssetHandle> materialHandles = ExtractMaterials(model, filepath, renderer, assetManager);

    // Create Root Entity for the Model
    std::string scene_name = "ModelRoot";
    if (model.defaultScene > -1 && !model.scenes[model.defaultScene].name.empty()) {
        scene_name = model.scenes[model.defaultScene].name;
    }
    
    Entity scene_root = scene->CreateEntity(scene_name);

    // Start parsing
    const auto& gltfScene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
    for (int root_node_index : gltfScene.nodes) {
        ProcessNode(scene, renderer, assetManager, model, root_node_index, scene_root, materialHandles);
    }

    return scene_root;
}