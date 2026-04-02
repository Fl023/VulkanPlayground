#include "SceneSerializer.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "Scene.hpp"
#include "AssetManager.hpp"
#include "Model.hpp"
#include "renderer/VulkanRenderer.hpp"

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& vec)
{
	out << YAML::Flow << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& quat)
{
	out << YAML::Flow << YAML::BeginSeq << quat.x << quat.y << quat.z << quat.w << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const UUID& uuid)
{
	out << uuid.ToString();
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const Camera::ProjectionType& projType)
{
	out << static_cast<int>(projType);
	return out;
}

namespace YAML {

	// --- VEC3 DECODER ---
	template<>
	struct convert<glm::vec3> {
		static Node encode(const glm::vec3& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs) {
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	// --- QUAT DECODER ---
	template<>
	struct convert<glm::quat> {
		static Node encode(const glm::quat& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::quat& rhs) {
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	// --- UUID DECODER ---
	template<>
	struct convert<UUID> {
		static Node encode(const UUID& rhs) {
			Node node;
			node = rhs.ToString();
			return node;
		}

		static bool decode(const Node& node, UUID& rhs) {
			if (!node.IsScalar())
				return false;

			// Uses your string constructor!
			rhs = UUID(node.as<std::string>());
			return true;
		}
	};
}


UUID SceneSerializer::GetUUIDFromEntity(Entity entity) {
	if (!entity) return { 0, 0 };

	return entity.GetComponent<IDComponent>().ID;
}

bool SceneSerializer::IsChildOfModelInstance(Entity entity) {
	if (!entity.HasComponent<RelationshipComponent>()) return false;

	entt::entity parent = entity.GetComponent<RelationshipComponent>().Parent;
	while (parent != entt::null) {
		Entity p{ parent, m_Scene };
		if (p.HasComponent<ModelInstanceComponent>()) return true;
		parent = p.GetComponent<RelationshipComponent>().Parent;
	}
	return false;
}

SceneSerializer::SceneSerializer(Scene* scene, AssetManager* assetManager)
	: m_Scene(scene), m_AssetManager(assetManager)
{
}

void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
{
	out << YAML::BeginMap;
	auto& ID = entity.GetComponent<IDComponent>().ID;
	out << YAML::Key << "Entity" << YAML::Value << ID;

	if (entity.HasComponent<TagComponent>()) {
		out << YAML::Key << "TagComponent";
		out << YAML::BeginMap; 
		out << YAML::Key << "Tag" << YAML::Value << entity.GetComponent<TagComponent>().Tag;
		out << YAML::EndMap;
	}

	if (entity.HasComponent<RelationshipComponent>()) {
		out << YAML::Key << "RelationshipComponent";
		out << YAML::BeginMap;
		auto& rc = entity.GetComponent<RelationshipComponent>();
		out << YAML::Key << "Parent" << YAML::Value << GetUUIDFromEntity(Entity(rc.Parent, m_Scene));
		out << YAML::Key << "FirstChild" << YAML::Value << GetUUIDFromEntity(Entity(rc.FirstChild, m_Scene));
		out << YAML::Key << "PrevSibling" << YAML::Value << GetUUIDFromEntity(Entity(rc.PrevSibling, m_Scene));
		out << YAML::Key << "NextSibling" << YAML::Value << GetUUIDFromEntity(Entity(rc.NextSibling, m_Scene));
		out << YAML::EndMap;
	}

	if (entity.HasComponent<TransformComponent>()) {
		out << YAML::Key << "TransformComponent";
		out << YAML::BeginMap;
		auto& tc = entity.GetComponent<TransformComponent>();
		out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
		out << YAML::Key << "Rotation" << YAML::Value << tc.GetRotation();
		out << YAML::Key << "Scale" << YAML::Value << tc.Scale;
		out << YAML::EndMap;
	}

	if (entity.HasComponent<CameraComponent>()) {
		out << YAML::Key << "CameraComponent";
		out << YAML::BeginMap; // CameraComponent

		auto& camComp = entity.GetComponent<CameraComponent>();
		auto& cam = camComp.SceneCamera;

		out << YAML::Key << "Camera" << YAML::Value;
		out << YAML::BeginMap; // Camera
		out << YAML::Key << "Name" << YAML::Value << cam.GetName();
		out << YAML::Key << "ProjectionType" << YAML::Value << cam.GetProjectionType();
		out << YAML::Key << "PerspectiveFOV" << YAML::Value << cam.GetPerspectiveVerticalFOV();
		out << YAML::Key << "OrthographicSize" << YAML::Value << cam.GetOrthographicSize();
		out << YAML::Key << "AspectRatio" << YAML::Value << cam.GetAspectRatio();
		out << YAML::Key << "NearClip" << YAML::Value << cam.GetNearClip();
		out << YAML::Key << "FarClip" << YAML::Value << cam.GetFarClip();
		out << YAML::EndMap; // Camera

		out << YAML::Key << "Primary" << YAML::Value << camComp.Primary;
		out << YAML::Key << "FixedAspectRatio" << YAML::Value << camComp.FixedAspectRatio;

		out << YAML::EndMap; // CameraComponent
	}

	if (!entity.HasComponent<ModelInstanceComponent>())
	{
		if (entity.HasComponent<MeshComponent>()) {
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap;
			auto& mc = entity.GetComponent<MeshComponent>();
			out << YAML::Key << "MeshHandle" << YAML::Value << mc.MeshHandle;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<MaterialComponent>()) {
			out << YAML::Key << "MaterialComponent";
			out << YAML::BeginMap;
			auto& mat = entity.GetComponent<MaterialComponent>();
			out << YAML::Key << "MaterialHandle" << YAML::Value << mat.MaterialHandle;
			out << YAML::EndMap;
		}
	}
	else
	{
		out << YAML::Key << "ModelInstanceComponent";
		out << YAML::BeginMap;
		auto& mic = entity.GetComponent<ModelInstanceComponent>();
		out << YAML::Key << "ModelHandle" << YAML::Value << mic.ModelHandle;
		out << YAML::EndMap;
	}

	if (entity.HasComponent<SkyboxComponent>()) {
		out << YAML::Key << "SkyboxComponent";
		out << YAML::BeginMap;
		auto& sb = entity.GetComponent<SkyboxComponent>();
		out << YAML::Key << "CubemapHandle" << YAML::Value << sb.CubemapHandle;
		out << YAML::EndMap;
	}

	out << YAML::EndMap;
}

void SceneSerializer::SerializeAssetRegistry(YAML::Emitter& out)
{
	out << YAML::Key << "AssetRegistry" << YAML::Value << YAML::BeginMap;

	// --- TEXTURES ---
	out << YAML::Key << "Textures" << YAML::Value << YAML::BeginSeq;
	for (const auto& [registryName, handle] : m_AssetManager->GetTextureRegistry()) {
		Texture* texture = m_AssetManager->GetTexture(handle);
		if (!texture) continue;

		out << YAML::BeginMap;
		out << YAML::Key << "Handle" << YAML::Value << handle;
		out << YAML::Key << "Name" << YAML::Value << registryName; // <--- The Registry Name!
		out << YAML::Key << "IsCubemap" << YAML::Value << texture->IsCubemap();

		if (texture->IsCubemap()) {
			out << YAML::Key << "Filepaths" << YAML::Value << YAML::BeginSeq;
			for (const auto& path : texture->GetFilePaths()) {
				out << path;
			}
			out << YAML::EndSeq;
		}
		else {
			out << YAML::Key << "Filepath" << YAML::Value << texture->GetFilePath();
		}
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	// --- MODELS ---
	out << YAML::Key << "Models" << YAML::Value << YAML::BeginSeq;
	for (const auto& [registryName, handle] : m_AssetManager->GetModelRegistry()) {
		Model* model = m_AssetManager->GetModel(handle);
		if (!model) continue;

		out << YAML::BeginMap;
		out << YAML::Key << "Handle" << YAML::Value << handle;
		out << YAML::Key << "Name" << YAML::Value << registryName; // <--- The Registry Name!
		out << YAML::Key << "Filepath" << YAML::Value << model->GetFilePath();
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	// --- MATERIALS ---
	out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
	for (const auto& [registryName, handle] : m_AssetManager->GetMaterialRegistry()) {
		Material* material = m_AssetManager->GetMaterial(handle);
		if (!material) continue;

		out << YAML::BeginMap;
		out << YAML::Key << "Handle" << YAML::Value << handle;
		out << YAML::Key << "Name" << YAML::Value << registryName; // <--- The Registry Name!
		out << YAML::Key << "AlbedoTexture" << YAML::Value << material->GetTextureHandle();
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	// --- MESHES ---
	out << YAML::Key << "Meshes" << YAML::Value << YAML::BeginSeq;
	for (const auto& [registryName, handle] : m_AssetManager->GetMeshRegistry()) {
		// Only save the runtime built-in meshes! 
		// The meshes belonging to Models will be auto-recreated when the Model Instantiates.
		if (registryName == "Cube" || registryName == "Square") {
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "Name" << YAML::Value << registryName;
			out << YAML::Key << "Type" << YAML::Value << "Builtin";
			out << YAML::EndMap;
		}
	}
	out << YAML::EndSeq;

	out << YAML::EndMap; // End AssetRegistry
}


void SceneSerializer::Serialize(const std::string& filepath)
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Scene" << YAML::Value << "Untitled";

	SerializeAssetRegistry(out);

	out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
	for (auto entity : m_Scene->m_Registry.view<IDComponent>())
	{
		Entity e{ entity, m_Scene };
		if (!e || IsChildOfModelInstance(e))
			continue;

		SerializeEntity(out, e);
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;

	std::ofstream fout(filepath);
	fout << out.c_str();
}

void SceneSerializer::SerializeRuntime(const std::string& filepath)
{
}

bool SceneSerializer::Deserialize(const std::string& filepath, VulkanRenderer& renderer)
{
	std::ifstream stream(filepath);
	std::stringstream strStream;
	strStream << stream.rdbuf();

	YAML::Node data = YAML::Load(strStream.str());
	if (!data["Scene"])
	{
		std::cerr << "Invalid scene file: Missing 'Scene' node." << std::endl;
		return false;
	}

	std::string sceneName = data["Scene"].as<std::string>();
	std::cout << "Deserializing scene: " << sceneName << std::endl;

	// AssetRegistry
	auto assetRegistryNode = data["AssetRegistry"];
	if (assetRegistryNode) {
		auto texturesNode = assetRegistryNode["Textures"];
		if (texturesNode) {
			for (const auto& texNode : texturesNode) {
				AssetHandle handle = texNode["Handle"].as<AssetHandle>(); // Get the saved handle!
				std::string name = texNode["Name"].as<std::string>();
				bool isCubemap = texNode["IsCubemap"] ? texNode["IsCubemap"].as<bool>() : false;

				if (isCubemap) {
					std::array<std::string, 6> facePaths;
					auto pathsNode = texNode["Filepaths"];
					for (int i = 0; i < 6; ++i) {
						facePaths[i] = pathsNode[i].as<std::string>();
					}
					m_AssetManager->LoadCubemapWithHandle(handle, renderer, name, facePaths);
				}
				else {
					std::string filepath = texNode["Filepath"].as<std::string>();
					m_AssetManager->LoadOrCreateTextureWithHandle(handle, renderer, name, filepath);
				}
			}
		}

		auto modelsNode = assetRegistryNode["Models"];
		if (modelsNode) {
			for (const auto& modelNode : modelsNode) {
				AssetHandle handle = modelNode["Handle"].as<AssetHandle>(); // Get the saved handle!
				std::string name = modelNode["Name"].as<std::string>();
				std::string filepath = modelNode["Filepath"].as<std::string>();

				m_AssetManager->LoadModelWithHandle(handle, renderer, name, filepath);
			}
		}

		auto materialsNode = assetRegistryNode["Materials"];
		if (materialsNode) {
			for (const auto& matNode : materialsNode) {
				AssetHandle handle = matNode["Handle"].as<AssetHandle>(); // Get the saved handle!
				std::string name = matNode["Name"].as<std::string>();
				AssetHandle albedoHandle = matNode["AlbedoTexture"].as<AssetHandle>();

				MaterialRenderState state{};
				vk::Format targetFormat = renderer.GetSwapchainFormat();
				m_AssetManager->CreateMaterialWithHandle(handle, renderer, name, state, targetFormat, albedoHandle);
			}
		}
	}

	// ==========================================
	// ENTITIES (Two-Pass System)
	// ==========================================
	auto entitiesNode = data["Entities"];
	if (!entitiesNode) return true;

	bool sceneHasPrimaryCamera = false;
	auto cameraView = m_Scene->m_Registry.view<CameraComponent>();
	for (auto entity : cameraView) {
		if (cameraView.get<CameraComponent>(entity).Primary) {
			sceneHasPrimaryCamera = true;
			break;
		}
	}

	// Temporary dictionary to hold the EnTT handles while we rebuild the tree
	std::unordered_map<UUID, entt::entity> spawnedEntities;
	UUID invalidUUID = { 0, 0 };

	// --- PASS 1: Spawning & Components ---
	for (auto entityNode : entitiesNode)
	{
		UUID uuid = entityNode["Entity"].as<UUID>();
		std::string name = "Unnamed Entity";

		if (auto tagNode = entityNode["TagComponent"]) {
			name = tagNode["Tag"].as<std::string>();
		}

		// Create the entity and FORCE its UUID to match the save file
		Entity spawnedEntity = m_Scene->CreateEntity(name);
		spawnedEntity.GetComponent<IDComponent>().ID = uuid;

		// Load Transform
		if (auto transformNode = entityNode["TransformComponent"]) {
			auto& tc = spawnedEntity.GetComponent<TransformComponent>();
			tc.Translation = transformNode["Translation"].as<glm::vec3>();
			tc.SetRotation(transformNode["Rotation"].as<glm::quat>());
			tc.Scale = transformNode["Scale"].as<glm::vec3>();
		}

		// Load Camera
		if (auto cameraCompNode = entityNode["CameraComponent"]) {
			auto& cc = spawnedEntity.AddComponent<CameraComponent>();
			bool savedAsPrimary = cameraCompNode["Primary"].as<bool>();

			// BUG FIX: Prevent multiple primary cameras!
			if (savedAsPrimary && sceneHasPrimaryCamera) {
				cc.Primary = false; // Downgrade to secondary
				std::cout << "Notice: A Primary Camera already exists. Loaded camera downgraded to secondary.\n";
			}
			else if (savedAsPrimary) {
				cc.Primary = true;
				sceneHasPrimaryCamera = true; // Mark that we now have one!
			}
			else {
				cc.Primary = false;
			}
			cc.FixedAspectRatio = cameraCompNode["FixedAspectRatio"].as<bool>();

			if (auto cameraNode = cameraCompNode["Camera"]) {
				float aspectRatio = cameraNode["AspectRatio"].as<float>();
				float verticalFOV = cameraNode["PerspectiveFOV"].as<float>();
				float orthoSize = cameraNode["OrthographicSize"].as<float>();
				float nearClip = cameraNode["NearClip"].as<float>();
				float farClip = cameraNode["FarClip"].as<float>();

				cc.SceneCamera.SetPerspective(verticalFOV, aspectRatio, nearClip, farClip);
				cc.SceneCamera.SetOrthographic(orthoSize, nearClip, farClip);
				cc.SceneCamera.SetProjectionType(static_cast<Camera::ProjectionType>(cameraNode["ProjectionType"].as<int>()));
			}
		}

		// Load Asset Handles (These are guaranteed to be valid now because of the Registry phase!)
		if (auto meshNode = entityNode["MeshComponent"]) {
			spawnedEntity.AddComponent<MeshComponent>(meshNode["MeshHandle"].as<AssetHandle>());
		}

		if (auto matNode = entityNode["MaterialComponent"]) {
			spawnedEntity.AddComponent<MaterialComponent>(matNode["MaterialHandle"].as<AssetHandle>());
		}

		if (auto skyNode = entityNode["SkyboxComponent"]) {
			spawnedEntity.AddComponent<SkyboxComponent>(skyNode["CubemapHandle"].as<AssetHandle>());
		}

		if (auto modelInstanceNode = entityNode["ModelInstanceComponent"]) {
			AssetHandle modelHandle = modelInstanceNode["ModelHandle"].as<AssetHandle>();
			spawnedEntity.AddComponent<ModelInstanceComponent>(modelHandle);

			Model* model = m_AssetManager->GetModel(modelHandle);
			if (model) {
				model->Instantiate(m_Scene, modelHandle, spawnedEntity);
			}
		}

		// Record it in the dictionary so Pass 2 can find it
		spawnedEntities[uuid] = spawnedEntity;
	}

	// --- PASS 2: Reconstruct Relationships ---
	for (auto entityNode : entitiesNode)
	{
		UUID uuid = entityNode["Entity"].as<UUID>();
		auto relNode = entityNode["RelationshipComponent"];
		if (!relNode) continue;

		entt::entity currentEntt = spawnedEntities[uuid];
		auto& rel = m_Scene->m_Registry.get<RelationshipComponent>(currentEntt);

		UUID parentID = relNode["Parent"].as<UUID>();
		UUID firstChildID = relNode["FirstChild"].as<UUID>();
		UUID prevSiblingID = relNode["PrevSibling"].as<UUID>();
		UUID nextSiblingID = relNode["NextSibling"].as<UUID>();

		// Safely map the permanent UUIDs back into EnTT integer IDs
		if (parentID != invalidUUID && spawnedEntities.count(parentID)) rel.Parent = spawnedEntities[parentID];
		if (firstChildID != invalidUUID && spawnedEntities.count(firstChildID)) rel.FirstChild = spawnedEntities[firstChildID];
		if (prevSiblingID != invalidUUID && spawnedEntities.count(prevSiblingID)) rel.PrevSibling = spawnedEntities[prevSiblingID];
		if (nextSiblingID != invalidUUID && spawnedEntities.count(nextSiblingID)) rel.NextSibling = spawnedEntities[nextSiblingID];
	}

	return true;

}

bool SceneSerializer::DeserializeRuntime(const std::string& filepath, VulkanRenderer& renderer)
{
	return false;
}

