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





// ==========================================================================================
// CONSTRUCTOR: Loads the file and builds the blueprint (GPU Assets ONLY, No ECS Entities yet)
// ==========================================================================================
Model::Model(VulkanRenderer& renderer, AssetManager& assetManager, const std::string& filepath) {
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool success = false;
    if (filepath.substr(filepath.find_last_of(".") + 1) == "glb") {
        success = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filepath);
    }
    else {
        success = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath);
    }

    if (!warn.empty()) std::cout << "glTF warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "glTF error: " << err << std::endl;
    if (!success) throw std::runtime_error("Failed to load glTF model: " + err);

    m_Name = "UnknownModel";
    if (gltfModel.defaultScene > -1 && !gltfModel.scenes[gltfModel.defaultScene].name.empty()) {
        m_Name = gltfModel.scenes[gltfModel.defaultScene].name;
    }
    else {
        // Fallback: Use the filename without the extension (e.g., "assets/models/Car.glb" -> "Car")
        std::filesystem::path path(filepath);
        m_Name = path.stem().string();
    }

    // 1. Extract and load all textures/materials into the AssetManager
    std::vector<AssetHandle> materialHandles = ExtractMaterials(gltfModel, filepath, renderer, assetManager);

    // 2. Build the flat Node Tree (Blueprint)
    const auto& gltfScene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
    for (int root_node_index : gltfScene.nodes) {
        int32_t rootIndex = ProcessNode(renderer, assetManager, gltfModel, root_node_index, -1, materialHandles);
        m_RootNodes.push_back(rootIndex);
    }
}

// ==========================================================================================
// INSTANTIATION: Spawns real EnTT entities in the Scene based on the blueprint
// ==========================================================================================
Entity Model::Instantiate(Scene* scene, Entity parent) {
    // We create a single master root entity for this instance to keep the scene clean
    Entity instanceRoot = scene->CreateEntity(m_Name);
    if (parent) {
        parent.AddChild(instanceRoot);
    }

    // A recursive lambda to traverse our flat m_Nodes array and build ECS entities
    auto instantiate_node = [&](auto& self, int32_t nodeIndex, Entity currentParent) -> void {
        if (nodeIndex == -1) return;

        const ModelNode& node = m_Nodes[nodeIndex];

        // 1. Create the entity
        Entity e = scene->CreateEntity(node.Name);

        // 2. Assign the blueprint's transform
        e.GetComponent<TransformComponent>().SetLocalTransform(node.LocalTransform);

        // 3. Attach mesh and material if they exist
        if (node.MeshHandle != INVALID_ASSET_HANDLE) {
            e.AddComponent<MeshComponent>(node.MeshHandle);
            e.AddComponent<MaterialComponent>(node.MaterialHandle);
        }

        // 4. Set up ECS hierarchy
        if (currentParent) {
            currentParent.AddChild(e);
        }

        // 5. Traverse down the tree
        self(self, node.FirstChild, e);               // First child gets 'e' as its parent
        self(self, node.NextSibling, currentParent);  // Next sibling shares 'currentParent'
        };

    // Kick off the recursive instantiation for all root nodes in the blueprint
    for (int32_t rootIndex : m_RootNodes) {
        instantiate_node(instantiate_node, rootIndex, instanceRoot);
    }

    return instanceRoot;
}

// ==========================================================================================
// NODE PROCESSING: Converts glTF hierarchy into our flat ModelNode array
// ==========================================================================================
int32_t Model::ProcessNode(VulkanRenderer& renderer, AssetManager& assetManager, const tinygltf::Model& model, int gltf_node_index, int32_t parent_index, const std::vector<AssetHandle>& materialHandles) {
    const auto& gltfNode = model.nodes[gltf_node_index];

    // 1. Initialize the base node
    ModelNode node;
    node.ID = UUID();
    node.Name = gltfNode.name.empty() ? "Node_" + std::to_string(gltf_node_index) : gltfNode.name;
    node.Parent = parent_index;

    // 2. Extract Transform into a local matrix
    if (gltfNode.matrix.size() == 16) {
        node.LocalTransform = glm::make_mat4(gltfNode.matrix.data());
    }
    else {
        glm::vec3 T(0.0f);
        glm::quat R(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 S(1.0f);

        if (gltfNode.translation.size() == 3) T = glm::vec3(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);
        if (gltfNode.rotation.size() == 4) R = glm::make_quat(gltfNode.rotation.data());
        if (gltfNode.scale.size() == 3) S = glm::vec3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);

        node.LocalTransform = glm::translate(glm::mat4(1.0f), T) * glm::mat4_cast(R) * glm::scale(glm::mat4(1.0f), S);
    }

    int32_t currentNodeIndex = static_cast<int32_t>(m_Nodes.size());
    m_Nodes.push_back(node);

    // 3. Process Primitives
    if (gltfNode.mesh >= 0) {
        const auto& mesh = model.meshes[gltfNode.mesh];

        // --- YOUR FIX: Check primitive count first ---
        if (mesh.primitives.size() == 1) {
            // Case 1: Only 1 primitive. Attach directly to the base node.
            const auto& primitive = mesh.primitives[0];

            std::vector<Vertex> vertices;
            std::vector<uint16_t> indices;
            ExtractPrimitiveGeometry(model, primitive, vertices, indices);

            auto vulkanMesh = std::make_unique<Mesh>(renderer.getDevice(), m_Nodes[currentNodeIndex].Name, vertices, indices);
            m_Nodes[currentNodeIndex].MeshHandle = assetManager.AddMesh(m_Nodes[currentNodeIndex].Name, std::move(vulkanMesh));

            if (primitive.material >= 0 && primitive.material < materialHandles.size()) {
                m_Nodes[currentNodeIndex].MaterialHandle = materialHandles[primitive.material];
            }
        }
        else if (mesh.primitives.size() > 1) {
            // Case 2: Multiple primitives. Base node stays empty (Transform only). Create children.
            int primitive_index = 0;
            int32_t previousExtraChild = -1;

            for (const auto& primitive : mesh.primitives) {
                std::string prim_name = m_Nodes[currentNodeIndex].Name + "_Prim" + std::to_string(primitive_index);

                std::vector<Vertex> vertices;
                std::vector<uint16_t> indices;
                ExtractPrimitiveGeometry(model, primitive, vertices, indices);

                auto vulkanMesh = std::make_unique<Mesh>(renderer.getDevice(), prim_name, vertices, indices);
                AssetHandle meshHandle = assetManager.AddMesh(prim_name, std::move(vulkanMesh));

                AssetHandle matHandle = INVALID_ASSET_HANDLE;
                if (primitive.material >= 0 && primitive.material < materialHandles.size()) {
                    matHandle = materialHandles[primitive.material];
                }

                ModelNode extraNode;
                extraNode.ID = UUID();
                extraNode.Name = prim_name;
                extraNode.Parent = currentNodeIndex;
                extraNode.MeshHandle = meshHandle;
                extraNode.MaterialHandle = matHandle;
                // LocalTransform remains Identity, so it perfectly inherits the base node's transform

                int32_t extraNodeIndex = static_cast<int32_t>(m_Nodes.size());
                m_Nodes.push_back(extraNode);

                // Link sibling hierarchy for these extra nodes
                if (previousExtraChild == -1) {
                    m_Nodes[currentNodeIndex].FirstChild = extraNodeIndex;
                }
                else {
                    m_Nodes[previousExtraChild].NextSibling = extraNodeIndex;
                }
                previousExtraChild = extraNodeIndex;

                primitive_index++;
            }
        }
    }

    // 4. Recurse for actual glTF children
    int32_t previousChild = -1;

    // If we just added extra primitive children, we need to find the END of the sibling list
    // so we can append the actual glTF children after them.
    if (m_Nodes[currentNodeIndex].FirstChild != -1) {
        previousChild = m_Nodes[currentNodeIndex].FirstChild;
        while (m_Nodes[previousChild].NextSibling != -1) {
            previousChild = m_Nodes[previousChild].NextSibling;
        }
    }

    for (int child_index : gltfNode.children) {
        int32_t childNodeIndex = ProcessNode(renderer, assetManager, model, child_index, currentNodeIndex, materialHandles);

        if (previousChild == -1) {
            m_Nodes[currentNodeIndex].FirstChild = childNodeIndex;
        }
        else {
            m_Nodes[previousChild].NextSibling = childNodeIndex;
        }
        previousChild = childNodeIndex;
    }

    return currentNodeIndex;
}

// ==========================================================================================
// HELPER: Extracts materials and textures into AssetManager
// ==========================================================================================
std::vector<AssetHandle> Model::ExtractMaterials(const tinygltf::Model& model, const std::string& filepath, VulkanRenderer& renderer, AssetManager& assetManager) {
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

        if (gltfMat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            targetTextureIndex = gltfMat.pbrMetallicRoughness.baseColorTexture.index;
        }
        else if (gltfMat.extensions.find("KHR_materials_pbrSpecularGlossiness") != gltfMat.extensions.end()) {
            const auto& ext = gltfMat.extensions.at("KHR_materials_pbrSpecularGlossiness");
            if (ext.Has("diffuseTexture")) {
                targetTextureIndex = ext.Get("diffuseTexture").Get("index").GetNumberAsInt();
            }
        }

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

        AssetHandle matHandle = assetManager.CreateMaterial(matName, albedoHandle);
        materialHandles.push_back(matHandle);
    }

    return materialHandles;
}

// ==========================================================================================
// HELPER: Extracts raw geometry data from a glTF Primitive
// ==========================================================================================
void Model::ExtractPrimitiveGeometry(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<Vertex>& out_vertices, std::vector<uint16_t>& out_indices) {
    const float* positionData = nullptr;
    const float* texCoordData = nullptr;
    size_t vertexCount = 0;

    // Extract Vertices
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
        if (positionData) {
            vertex.pos = { positionData[i * 3 + 0], positionData[i * 3 + 1], positionData[i * 3 + 2] };
        }
        vertex.color = { 1.0f, 1.0f, 1.0f }; // Default to white
        if (texCoordData) {
            vertex.texCoord = { texCoordData[i * 2 + 0], texCoordData[i * 2 + 1] };
        }
        out_vertices.push_back(vertex);
    }

    // Extract Indices
    if (primitive.indices >= 0) {
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
        const void* dataPtr = &(indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);

        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* indices = static_cast<const uint16_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) out_indices.push_back(indices[i]);
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* indices = static_cast<const uint32_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) out_indices.push_back(static_cast<uint16_t>(indices[i]));
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const uint8_t* indices = static_cast<const uint8_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) out_indices.push_back(static_cast<uint16_t>(indices[i]));
        }
    }
}