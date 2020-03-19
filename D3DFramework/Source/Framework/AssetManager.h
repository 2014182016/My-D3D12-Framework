#pragma once

#include "pch.h"
#include "Enumeration.h"
#include "Structure.h"

#define DISABLED -1
#define TEX_NUM 12

class AssetManager
{
public:
	AssetManager();
	AssetManager& operator=(const AssetManager& rhs) = delete;
	~AssetManager();

public:
	static AssetManager* GetInstance();

public:
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, IDirectSound8* d3dSound);

	// 해당 value를 찾기 위한 함수
	class Mesh* FindMesh(std::string&& name) const;
	class Material* FindMaterial(std::string&& name) const;
	class Sound* FindSound(std::string&& name) const;
	struct Texture* FindTexture(std::string&& name) const;

	// Shader Resource View를 만들기 위한 texNames의 인덱스에 따라 텍스처의 리소스를 돌려주는 함수
	ID3D12Resource* GetTextureResource(UINT index) const;

public:
	std::unordered_map<std::string, std::unique_ptr<class Mesh>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<class Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<struct Texture>> mTextures;
	std::unordered_map<std::string, std::unique_ptr<class Sound>> mSounds;

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildMaterial();

private:
	static inline AssetManager* instance = nullptr;
	static inline bool isInitialized = false;

	const std::vector<TextureInfo> mTexInfos =
	{
		{"Bricks2", L"Textures/Bricks2.dds" },
		{"Bricks2_nmap", L"Textures/Bricks2_nmap.dds" },
		{"Tile", L"Textures/Tile.dds" },
		{"Tile_nmap", L"Textures/Tile_nmap.dds"},
		{"Ice", L"Textures/Ice.dds"},
		{"Default_nmap", L"Textures/Default_nmap.dds"},
		{"WireFence", L"Textures/WireFence.dds"},
		{"Tree01S", L"Textures/Tree01S.dds"},
		{"Clouds", L"Textures/Clouds.dds"},
		{"Radial_Gradient", L"Textures/Radial_Gradient.dds"},
		{"HeightMap", L"Textures/HeightMap.dds"},
		{"Grass", L"Textures/Grass.dds"},
	};

	const std::vector<struct MeshInfo> mH3dModels =
	{
		{ "Cube_AABB",  L"Models/Cube.h3d", CollisionType::AABB },
		{ "Cube_OBB",  L"Models/Cube.h3d", CollisionType::OBB },
		{ "Sphere",  L"Models/Sphere.h3d", CollisionType::Sphere },
		{ "SkySphere",  L"Models/Sphere.h3d", CollisionType::None },
		{ "Cylinder",  L"Models/Cylinder.h3d", CollisionType::AABB },
	};

	const std::vector<SoundInfo> mSoundInfos =
	{
		{"Sound01", "Sounds/sound01.wav", SoundType::Sound2D},
		{"Sound02", "Sounds/sound02.wav", SoundType::Sound3D},
	};
};