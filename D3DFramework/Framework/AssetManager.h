#pragma once

#include "pch.h"
#include "Enums.h"

class AssetManager
{
public:
	AssetManager();
	AssetManager& operator=(const AssetManager& rhs) = delete;
	~AssetManager();

public:
	static AssetManager* GetInstance() { return mInstance; }

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	// �ش� value�� ã�� ���� �Լ�
	struct Texture* FindTexture(std::string&& name) const;
	struct Texture* FindCubeTexture(std::string&& name) const;
	class MeshGeometry* FindMesh(std::string&& name) const;
	class Material* FindMaterial(std::string&& name) const;

	// Shader Resource View�� ����� ���� texNames�� �ε����� ���� �ؽ�ó�� ���ҽ��� �����ִ� �Լ�
	ID3D12Resource* GetTextureResource(UINT index) const;
	ID3D12Resource* GetCubeTextureResource(UINT index) const;

public:
	inline const std::unordered_map<std::string, std::unique_ptr<class MeshGeometry>>& GetMeshes() const { return mMeshes; }
	inline const std::unordered_map<std::string, std::unique_ptr<class Material>>& GetMaterials() const { return mMaterials; }
	inline const std::unordered_map<std::string, std::unique_ptr<Texture>>& GetTextures() const { return mTextures; }
	inline const std::unordered_map<std::string, std::unique_ptr<Texture>>& GetCubeTextures() const { return mCubeTextures; }

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void LoadObj(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::tuple<std::string, std::wstring, CollisionType> modelInfo);
	void LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::tuple<std::string, std::wstring, CollisionType> modelInfo);
	void ObjToH3d(const std::vector<struct Vertex>& vertices, const std::vector<std::uint16_t> indices, std::wstring fileName) const;
	void CalculateTBN(const struct VertexBasic& v1, const struct VertexBasic& v2, const struct VertexBasic& v3,
		DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);
	void CreateMeshGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
		std::vector<struct Vertex>& vertices, std::vector<std::uint16_t> indices, CollisionType collisionType, std::string meshName);
	void BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildMaterial();

	// ���ĵ� TexName���κ��� index�� �����ִ� �Լ�
	int FindTextureIndex(std::string&& name) const;
	int FindCubeTextureIndex(std::string&& name) const;

private:
	static AssetManager* mInstance;

	std::unordered_map<std::string, std::unique_ptr<class MeshGeometry>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<class Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mCubeTextures;

	// �ؽ�ó�� �̸��� ��θ� pair�� ����
	// Material�� ����ϱ� ���� Index�� ���� vector�� �����Ѵ�.
	// �ؽ����� ���� ���Ѵٸ� Common.hlsl�� Texture2D�� �迭 ũ�⸦ �Ȱ����� �����Ѵ�.
	const std::vector<std::pair<std::string, std::wstring>> mTexNames =
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
	const std::vector<std::pair<std::string, std::wstring>> mCubeTexNames =
	{
		{"Grasscube1024", L"Textures/Grasscube1024.dds"},
	};

	// h3d ���� �̸��� �Բ� �浹 Ÿ���� �����Ѵ�.
	const std::vector<std::tuple<std::string, std::wstring, CollisionType>> mH3dModels =
	{
		{ "Cube_AABB",  L"Models/Cube.h3d", CollisionType::AABB },
		{ "Cube_OBB",  L"Models/Cube.h3d", CollisionType::OBB },
		{ "Sphere",  L"Models/Sphere.h3d", CollisionType::Sphere },
		{ "SkySphere",  L"Models/Sphere.h3d", CollisionType::None },
		{ "Cylinder",  L"Models/Cylinder.h3d", CollisionType::AABB },
	};

	// obj ���� �̸��� �Բ� �浹 Ÿ���� �����Ѵ�.
	// obj ���� ����Ʈ�԰� ���ÿ� h3d ���Ϸ� ��ȯ�Ѵ�.
	const std::vector<std::tuple<std::string, std::wstring, CollisionType>> mObjModels =
	{
		// { "Sphere",  L"Models/Sphere.obj", CollisionType::Sphere },
	};
};