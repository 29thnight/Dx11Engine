#pragma once
#include "Core.Definition.h"
#include "Core.Mathf.h"
#include "Texture.h"

struct Material final
{
	Material(const std::string_view& name) : name(name) {}

	std::string name;
	union
	{
		struct
		{
			Texture* albedo;
			Texture* normal;
			Texture* metallic;
			Texture* roughness;
			Texture* ao;
			Texture* emissive;
		};
		Texture* textures[6]{};
	};
};

struct MaterialInstance final
{
	MaterialInstance(Material* pMaterial) : material(pMaterial) {}

	bool useAlbecoTexture{ false };
	bool useNormalTexture{ false };
	bool useMetallicTexture{ false };
	bool useRoughnessTexture{ false };
	bool useAOTexture{ false };
	bool useEmissiveTexture{ false };

	Mathf::Color albedoColor{ 1.f, 1.f, 1.f, 1.f };
	float metallic{};
	float roughness{};
	float ao{};
	Mathf::Color emissiveColor{};

	Material* material;
};