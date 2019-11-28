#pragma once

#include "pch.h"

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
	struct Texture* FindTexture(std::string name) const;
	class MeshGeometry* FindMesh(std::string name) const;
	class Material* FindMaterial(std::string name) const;

	// Shader Resource View�� ����� ���� texNames�� �ε����� ���� �ؽ�ó�� ���ҽ��� �����ִ� �Լ�
	ID3D12Resource* GetTextureResource(UINT index) const;

public:
	inline const std::unordered_map<std::string, std::unique_ptr<class MeshGeometry>>& GetMeshes() const { return mMeshes; }
	inline const std::unordered_map<std::string, std::unique_ptr<class Material>>& GetMaterials() const { return mMaterials; }
	inline const std::unordered_map<std::string, std::unique_ptr<Texture>>& GetTextures() const { return mTextures; }

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildMaterial();

	// ���ĵ� TexName���κ��� index�� �����ִ� �Լ�
	int FindTextureIndex(std::string name) const;

private:
	static AssetManager* mInstance;

	std::unordered_map<std::string, std::unique_ptr<class MeshGeometry>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<class Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

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
};