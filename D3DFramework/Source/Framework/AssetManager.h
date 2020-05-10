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
�����ӿ�ũ�� ���Ǵ� ���ҽ����� �����Ѵ�.
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
	// ���� �ؽ�ó, �޽�, ����, ���͸��� ���� �ε��Ѵ�.
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, IDirectSound8* d3dSound);

	// �ش� value�� ã�� ���� �Լ�
	Mesh* FindMesh(std::string&& name) const;
	Material* FindMaterial(std::string&& name) const;
	Sound* FindSound(std::string&& name) const;
	Texture* FindTexture(std::string&& name) const;

	// Shader Resource View�� ����� ���� texNames�� �ε����� ���� �ؽ�ó�� ���ҽ��� �����ִ� �Լ�
	ID3D12Resource* GetTextureResource(const UINT32 index) const;

	// �޽����� ���� ���ε� ���۵��� �����Ѵ�.
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

	// �ؽ�ó �̸�, ���� �̸� ������ ���ǵȴ�.
	const std::vector<TextureInfo> mTexInfos =
	{
		{"Ice_D", L"Ice_D.dds"},
		{"Ice_N", L"Ice_N.dds"},
		{"Clouds", L"Clouds.dds"},
		{"Radial_Gradient", L"Radial_Gradient.dds"},
		{"HeightMap", L"HeightMap.dds"},
		{"Snow", L"Snow.dds"},
		{"Sword_D", L"Sword_D.dds"},
		{"Sword_N", L"Sword_N.dds"},
		{"Tree1", L"Tree1.dds"},
		{"Tree2", L"Tree2.dds"},
		{"Tree3", L"Tree3.dds"},
		{"Rock1_D", L"Rock1_D.dds"},
		{"Rock1_N", L"Rock1_N.dds"},
		{"Rock2_D", L"Rock2_D.dds"},
		{"Rock2_N", L"Rock2_N.dds"},
	};

	// �޽� �̸�, ���� �̸�, �浹 Ÿ�� ������ ���ǵȴ�.
	const std::vector<MeshInfo> mH3dModels =
	{
		{ "Cube_AABB",  L"Cube.h3d", CollisionType::AABB },
		{ "Cube_OBB",  L"Cube.h3d", CollisionType::OBB },
		{ "Sphere",  L"Sphere.h3d", CollisionType::Sphere },
		{ "SkySphere",  L"Sphere.h3d", CollisionType::None },
		{ "Skull",  L"Skull.h3d", CollisionType::Sphere },
		{ "Sword",  L"Sword.h3d", CollisionType::AABB },
		{ "Rock1",  L"Rock1.h3d", CollisionType::AABB },
		{ "Rock2",  L"Rock2.h3d", CollisionType::AABB },
	};

	// ���� �̸�, ���� �̸�, ���� Ÿ�� ������ ���ǵȴ�.
	const std::vector<SoundInfo> mSoundInfos =
	{
		{"WinterWind", "wind-sand.wav", SoundType::Sound3D},
	};
};