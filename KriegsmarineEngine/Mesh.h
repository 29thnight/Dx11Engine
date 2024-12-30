#pragma once
#include "Core.Definition.h"
#include "Core.Mathf.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

struct Vertex
{
	Mathf::Vector3 Position{};	    // The position of the vertex
	Mathf::Vector2 UV{};           // UV Coordinate for texturing (soon)
	Mathf::Vector3 Normal{};       // Normal for lighting
	Mathf::Vector3 Tangent{};		// For normal mapping
};

struct MeshData
{
	ID3D11Buffer* vertexBuffer{};
	ID3D11Buffer* indexBuffer{};
	uint32 indexCount{};
	Mathf::xMatrix localTransform{ XMMatrixIdentity() };
	int parentIndex{ -1 };

	~MeshData()
	{
		Memory::SafeDelete(vertexBuffer);
		Memory::SafeDelete(indexBuffer);
	}
};

struct Mesh final
{
	std::vector<MeshData> meshHierarchy{};

	void AddMeshData(const MeshData& meshData)
	{
		meshHierarchy.push_back(meshData);
	}

	void Import3DModel(const std::string_view& path, ID3D11Device* pDevice)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.data(), aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded);
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	}

	void ProcessNode();
};