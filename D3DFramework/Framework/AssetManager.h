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

	// 해당 value를 찾기 위한 함수
	struct Texture* FindTexture(std::string name) const;
	class MeshGeometry* FindMesh(std::string name) const;
	class Material* FindMaterial(std::string name) const;

	// Shader Resource View를 만들기 위한 texNames의 인덱스에 따라 텍스처의 리소스를 돌려주는 함수
	ID3D12Resource* GetTextureResource(UINT index) const;

public:
	inline const std::unordered_map<std::string, std::unique_ptr<class MeshGeometry>>& GetMeshes() const { return mMeshes; }
	inline const std::unordered_map<std::string, std::unique_ptr<class Material>>& GetMaterials() const { return mMaterials; }
	inline const std::unordered_map<std::string, std::unique_ptr<Texture>>& GetTextures() const { return mTextures; }

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildMaterial();

	// 정렬된 TexName으로부터 index를 돌려주는 함수
	int FindTextureIndex(std::string name) const;

private:
	static AssetManager* mInstance;

	std::unordered_map<std::string, std::unique_ptr<class MeshGeometry>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<class Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	// 텍스처의 이름과 경로를 pair로 묶음
	// Material에 사용하기 위한 Index를 위해 vector로 선언한다.
	// 텍스쳐의 수가 변한다면 Common.hlsl의 Texture2D의 배열 크기를 똑같도록 수정한다.
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