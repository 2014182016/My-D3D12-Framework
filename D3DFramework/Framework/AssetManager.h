#pragma once

#include "pch.h"
#include "Enums.h"

using TextureInfo = std::pair<std::string, std::wstring>;
using MeshInfo = std::tuple<std::string, std::wstring, CollisionType>;
using SoundInfo = std::tuple<std::string, std::string, SoundType>;

class AssetManager
{
public:
	AssetManager();
	AssetManager& operator=(const AssetManager& rhs) = delete;
	~AssetManager();

public:
	static AssetManager* GetInstance() { return mInstance; }

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, IDirectSound8* d3dSound);

	// �ش� value�� ã�� ���� �Լ�
	class MeshGeometry* FindMesh(std::string&& name) const;
	class Material* FindMaterial(std::string&& name) const;
	class Sound* FindSound(std::string&& name) const;

	// Shader Resource View�� ����� ���� texNames�� �ε����� ���� �ؽ�ó�� ���ҽ��� �����ִ� �Լ�
	ID3D12Resource* GetTextureResource(UINT index) const;
	ID3D12Resource* GetCubeTextureResource(UINT index) const;

public:
	inline const std::unordered_map<std::string, std::unique_ptr<class MeshGeometry>>& GetMeshes() const { return mMeshes; }
	inline const std::unordered_map<std::string, std::unique_ptr<class Material>>& GetMaterials() const { return mMaterials; }
	inline const std::unordered_map<std::string, std::unique_ptr<struct Texture>>& GetTextures() const { return mTextures; }
	inline const std::unordered_map<std::string, std::unique_ptr<struct Texture>>& GetCubeTextures() const { return mCubeTextures; }
	inline const std::unordered_map<std::string, std::unique_ptr<class Sound>>& GetSounds() const { return mSounds; }

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void LoadWaves(IDirectSound8* d3dSound);
	void LoadObj(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, MeshInfo modelInfo);
	void LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, MeshInfo modelInfo);
	void LoadWave(IDirectSound8* d3dSound, SoundInfo soundInfo);
	void ObjToH3d(const std::vector<struct Vertex>& vertices, const std::vector<std::uint16_t> indices, std::wstring fileName) const;
	void CalculateTBN(const struct VertexBasic& v1, const struct VertexBasic& v2, const struct VertexBasic& v3,
		DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);
	void BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildMaterial();

	int FindTextureIndex(std::string&& name) const;
	int FindCubeTextureIndex(std::string&& name) const;

private:
	static AssetManager* mInstance;

	std::unordered_map<std::string, std::unique_ptr<class MeshGeometry>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<class Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<struct Texture>> mTextures;
	std::unordered_map<std::string, std::unique_ptr<struct Texture>> mCubeTextures;
	std::unordered_map<std::string, std::unique_ptr<class Sound>> mSounds;

	// �ؽ�ó�� �̸��� ��θ� pair�� ����
	// Material�� ����ϱ� ���� Index�� ���� vector�� �����Ѵ�.
	// �ؽ����� ���� ���Ѵٸ� Common.hlsl�� Texture2D�� �迭 ũ�⸦ �Ȱ����� �����Ѵ�.
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
	};

	// ť�� ���� ���ҽ� �信�� �ٸ��� ��������� ������ ������ �ؽ�ó�� �и��Ѵ�.
	const std::vector<TextureInfo> mCubeTexInfos =
	{
		{"Grasscube1024", L"Textures/Grasscube1024.dds"},
	};

	// h3d ���� �̸��� �Բ� �浹 Ÿ���� �����Ѵ�.
	const std::vector<MeshInfo> mH3dModels =
	{
		{ "Cube_AABB",  L"Models/Cube.h3d", CollisionType::AABB },
		{ "Cube_OBB",  L"Models/Cube.h3d", CollisionType::OBB },
		{ "Sphere",  L"Models/Sphere.h3d", CollisionType::Sphere },
		{ "SkySphere",  L"Models/Sphere.h3d", CollisionType::None },
		{ "Cylinder",  L"Models/Cylinder.h3d", CollisionType::AABB },
	};

	// obj ���� �̸��� �Բ� �浹 Ÿ���� �����Ѵ�.
	// obj ���� ����Ʈ�԰� ���ÿ� h3d ���Ϸ� ��ȯ�Ѵ�.
	const std::vector<MeshInfo> mObjModels =
	{
		// { "Sphere",  L"Models/Sphere.obj", CollisionType::Sphere },
	};

	const std::vector<SoundInfo> mSoundInfos =
	{
		{"Sound01", "Sounds/sound01.wav", SoundType::Sound2D},
		{"Sound02", "Sounds/sound02.wav", SoundType::Sound3D},
	};
};