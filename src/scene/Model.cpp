#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "Model.hpp"
#include "Components.hpp"
#include "AssetManager.hpp"


// ==========================================================================================
// CONSTRUCTOR: Loads the file and builds the blueprint (GPU Assets ONLY, No ECS Entities yet)
// ==========================================================================================
Model::Model(VulkanRenderer& renderer, AssetManager& assetManager, const std::string& filepath)
	: m_Filepath(filepath), m_Renderer(renderer), m_AssetManager(assetManager)
{
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
        m_Name = path.stem().generic_string();
    }

    // 1. Extract and load all textures/materials into the AssetManager
    std::vector<AssetHandle> materialHandles = ExtractMaterials(gltfModel, filepath);

    // 2. Build the flat Node Tree (Blueprint)
    const auto& gltfScene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
    for (int root_node_index : gltfScene.nodes) {
        int32_t rootIndex = ProcessNode(gltfModel, root_node_index, -1, materialHandles);
        m_RootNodes.push_back(rootIndex);
    }
}

// ==========================================================================================
// INSTANTIATION: Spawns real EnTT entities in the Scene based on the blueprint
// ==========================================================================================
Entity Model::Instantiate(Scene* scene, AssetHandle modelHandle, Entity parent) {
    // We create a single master root entity for this instance to keep the scene clean
    Entity instanceRoot = scene->CreateEntity(m_Name);
    instanceRoot.AddComponent<ModelInstanceComponent>(modelHandle);
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
int32_t Model::ProcessNode(const tinygltf::Model& model, int gltf_node_index, int32_t parent_index, const std::vector<AssetHandle>& materialHandles) {
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
            std::vector<uint32_t> indices;
            ExtractPrimitiveGeometry(model, primitive, vertices, indices);

            auto vulkanMesh = std::make_unique<Mesh>(m_Renderer.getDevice(), m_Nodes[currentNodeIndex].Name, vertices, indices);
            m_Nodes[currentNodeIndex].MeshHandle = m_AssetManager.AddMesh(m_Nodes[currentNodeIndex].Name, std::move(vulkanMesh));

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
                std::vector<uint32_t> indices;
                ExtractPrimitiveGeometry(model, primitive, vertices, indices);

                auto vulkanMesh = std::make_unique<Mesh>(m_Renderer.getDevice(), prim_name, vertices, indices);
                AssetHandle meshHandle = m_AssetManager.AddMesh(prim_name, std::move(vulkanMesh));

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
        int32_t childNodeIndex = ProcessNode(model, child_index, currentNodeIndex, materialHandles);

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
std::vector<AssetHandle> Model::ExtractMaterials(const tinygltf::Model& model, const std::string& filepath) {
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
                std::string texPath = (basePath / image.uri).generic_string();
                albedoHandle = m_AssetManager.LoadOrCreateTexture(m_Renderer, image.uri, texPath);
            }
            else {
                std::cout << "Warning: Embedded textures require loading from memory!\n";
            }
        }

        MaterialRenderState state{};

        // 1. Read glTF Culling (Double Sided)
        if (gltfMat.doubleSided) {
            state.Culling = CullMode::None;
        }

        // 2. Read glTF Blending (Transparency)
        if (gltfMat.alphaMode == "BLEND") {
            state.Blending = BlendMode::AlphaBlend;
            state.Depth = DepthState::ReadOnly; // Transparent objects usually don't write to depth!
        }
        else if (gltfMat.alphaMode == "MASK") {
            // Alpha clipping usually remains opaque but discards pixels in the shader
            state.Blending = BlendMode::Opaque;
        }

        // 3. Get the target format (Assuming your Viewport uses the Swapchain format)
        vk::Format targetFormat = m_Renderer.GetSwapchainFormat();

        // 4. Create the material using the NEW signature!
        AssetHandle matHandle = m_AssetManager.CreateMaterial(m_Renderer, matName, state, targetFormat, albedoHandle);

        materialHandles.push_back(matHandle);
    }

    return materialHandles;
}

// ==========================================================================================
// HELPER: Extracts raw geometry data from a glTF Primitive
// ==========================================================================================
void Model::ExtractPrimitiveGeometry(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices) {
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
            for (size_t i = 0; i < indexAccessor.count; ++i) out_indices.push_back(static_cast<uint32_t>(indices[i]));
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* indices = static_cast<const uint32_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) out_indices.push_back(indices[i]);
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const uint8_t* indices = static_cast<const uint8_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) out_indices.push_back(static_cast<uint32_t>(indices[i]));
        }
    }
}