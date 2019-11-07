#pragma once

#include "pch.h"
#include "Global.h"

class AssetManager
{
public:
	AssetManager();
	~AssetManager();

public:
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

public:
	// �ش� value�� ã�� ���� �Լ�
	Texture* FindTexture(std::string name) const;
	MeshGeometry* FindMesh(std::string name) const;
	Material* FindMaterial(std::string name) const;

	// Shader Resource View�� ����� ���� texNames�� �ε����� ���� �ؽ�ó�� ���ҽ��� �����ִ� �Լ�
	ID3D12Resource* GetTextureResource(UINT index) const;

public:
	inline 	const std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& GetMeshes() const { return mMeshes; }
	inline 	const std::unordered_map<std::string, std::unique_ptr<Material>>& GetMaterials() const { return mMaterials; }
	inline 	const std::unordered_map<std::string, std::unique_ptr<Texture>>& GetTextures() const { return mTextures; }

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildMaterial();

	// ���ĵ� TexName���κ��� index�� �����ִ� �Լ�
	int FindTextureIndex(std::string name) const;

private:
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	// �ؽ�ó�� �̸��� ��θ� pair�� ����
	// Material�� ����ϱ� ���� Index�� ���� vector�� �����Ѵ�.
	const std::vector<std::pair<std::string, std::wstring>> texNames =
	{
		std::make_pair<std::string, std::wstring>("Bricks2", L"Textures/Bricks2.dds"),
		std::make_pair<std::string, std::wstring>("Bricks2_nmap", L"Textures/Bricks2_nmap.dds"),
		std::make_pair<std::string, std::wstring>("Tile", L"Textures/Tile.dds"),
		std::make_pair<std::string, std::wstring>("Tile_nmap", L"Textures/Tile_nmap.dds"),
		std::make_pair<std::string, std::wstring>("Ice", L"Textures/Ice.dds"),
		std::make_pair<std::string, std::wstring>("Default_nmap", L"Textures/Default_nmap.dds"),
		std::make_pair<std::string, std::wstring>("WireFence", L"Textures/WireFence.dds"),
		std::make_pair<std::string, std::wstring>("Tree01S", L"Textures/Tree01S.dds"),
	};
};