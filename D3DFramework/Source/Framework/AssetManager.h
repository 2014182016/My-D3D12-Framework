#pragma once

#include "Enumeration.h"
#include "AssetLoader.h"
#include <memory>
#include <unordered_map>
#include <wrl.h>

class Mesh;
class Material;
class Sound;
struct IDirectSound8;
struct ID3D12Resource;

struct Texture
{
	std::string name;
	std::wstring filename;
	UINT32 textureIndex;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap = nullptr;
};

/*
프레임워크에 사용되는 리소스들을 관리한다.
*/
class AssetManager
{
public:
	AssetManager();
	AssetManager& operator=(const AssetManager& rhs) = delete;
	~AssetManager();

public:
	static AssetManager* GetInstance();

public:
	// 각종 텍스처, 메쉬, 사운드, 머터리얼 등을 로드한다.
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, IDirectSound8* d3dSound);

	// 해당 value를 찾기 위한 함수
	Mesh* FindMesh(std::string&& name) const;
	Material* FindMaterial(std::string&& name) const;
	Sound* FindSound(std::string&& name) const;
	Texture* FindTexture(std::string&& name) const;

	// Shader Resource View를 만들기 위한 texNames의 인덱스에 따라 텍스처의 리소스를 돌려주는 함수
	ID3D12Resource* GetTextureResource(const UINT32 index) const;

	// 메쉬에서 사용된 업로드 버퍼들을 해제한다.
	void DisposeUploaders();

public:
	std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
	std::unordered_map<std::string, std::unique_ptr<Material>> materials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
	std::unordered_map<std::string, std::unique_ptr<Sound>> sounds;

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void LoadMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void LoadSounds(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, IDirectSound8* d3dSound);
	void BuildMaterial();

private:
	static inline bool isInitialized = false;

	// 텍스처 이름, 파일 이름 순으로 정의된다.
	const std::vector<TextureInfo> mTexInfos =
	{
		{"Bricks2", L"Bricks2.dds" },
		{"Bricks2_nmap", L"Bricks2_nmap.dds" },
		{"Tile", L"Tile.dds" },
		{"Tile_nmap", L"Tile_nmap.dds"},
		{"Ice", L"Ice.dds"},
		{"Default_nmap", L"Default_nmap.dds"},
		{"WireFence", L"WireFence.dds"},
		{"Tree01S", L"Tree01S.dds"},
		{"Clouds", L"Clouds.dds"},
		{"Radial_Gradient", L"Radial_Gradient.dds"},
		{"HeightMap", L"HeightMap.dds"},
		{"Grass", L"Grass.dds"},
	};

	// 메쉬 이름, 파일 이름, 충돌 타입 순으로 정의된다.
	const std::vector<MeshInfo> mH3dModels =
	{
		{ "Cube_AABB",  L"Cube.h3d", CollisionType::AABB },
		{ "Cube_OBB",  L"Cube.h3d", CollisionType::OBB },
		{ "Sphere",  L"Sphere.h3d", CollisionType::Sphere },
		{ "SkySphere",  L"Sphere.h3d", CollisionType::None },
		{ "Cylinder",  L"Cylinder.h3d", CollisionType::AABB },
		{ "Skull",  L"Skull.h3d", CollisionType::Sphere },
	};

	// 사운드 이름, 파일 이름, 사운드 타입 순으로 정의된다.
	const std::vector<SoundInfo> mSoundInfos =
	{
		{"Sound01", "sound01.wav", SoundType::Sound2D},
		{"Sound02", "sound02.wav", SoundType::Sound3D},
	};
};