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
	// 해당 value를 찾기 위한 함수
	Texture* FindTexture(std::string name) const;
	MeshGeometry* FindMesh(std::string name) const;
	Material* FindMaterial(std::string name) const;

	// Shader Resource View를 만들기 위한 texNames의 인덱스에 따라 텍스처의 리소스를 돌려주는 함수
	ID3D12Resource* GetTextureResource(UINT index) const;

public:
	inline 	const std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>& GetMeshes() const { return mMeshes; }
	inline 	const std::unordered_map<std::string, std::unique_ptr<Material>>& GetMaterials() const { return mMaterials; }
	inline 	const std::unordered_map<std::string, std::unique_ptr<Texture>>& GetTextures() const { return mTextures; }

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildMaterial();

	// 정렬된 TexName으로부터 index를 돌려주는 함수
	int FindTextureIndex(std::string name) const;

private:
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	// 텍스처의 이름과 경로를 pair로 묶음
	// Material에 사용하기 위한 Index를 위해 vector로 선언한다.
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