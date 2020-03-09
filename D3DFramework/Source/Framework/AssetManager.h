#pragma once

#include "pch.h"
#include "Enumeration.h"

#define DISABLED -1
#define TEX_NUM 10

class AssetManager
{
public:
	struct TextureInfo
	{
		std::string mName;
		std::wstring mFileName;
	};

	struct MeshInfo
	{
		std::string mName;
		std::wstring mFileName;
		CollisionType mCollisionType;
	};

	struct SoundInfo
	{
		std::string mName;
		std::string mFileName;
		SoundType mSoundType;
	};

public:
	AssetManager();
	AssetManager& operator=(const AssetManager& rhs) = delete;
	~AssetManager();

public:
	static AssetManager* GetInstance();

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, IDirectSound8* d3dSound);

	// 해당 value를 찾기 위한 함수
	class Mesh* FindMesh(std::string&& name) const;
	class Material* FindMaterial(std::string&& name) const;
	class Sound* FindSound(std::string&& name) const;
	struct Texture* FindTexture(std::string&& name) const;

	// Shader Resource View를 만들기 위한 texNames의 인덱스에 따라 텍스처의 리소스를 돌려주는 함수
	ID3D12Resource* GetTextureResource(UINT index) const;

public:
	inline const std::unordered_map<std::string, std::unique_ptr<class Mesh>>& GetMeshes() const { return mMeshes; }
	inline const std::unordered_map<std::string, std::unique_ptr<class Material>>& GetMaterials() const { return mMaterials; }
	inline const std::unordered_map<std::string, std::unique_ptr<struct Texture>>& GetTextures() const { return mTextures; }
	inline const std::unordered_map<std::string, std::unique_ptr<class Sound>>& GetSounds() const { return mSounds; }

private:
	void LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void LoadWaves(IDirectSound8* d3dSound);
	void LoadObj(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshInfo& modelInfo);
	void LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshInfo& modelInfo);
	void LoadWave(IDirectSound8* d3dSound,const SoundInfo& soundInfo);
	void ObjToH3d(const std::vector<struct Vertex>& vertices, const std::vector<std::uint16_t> indices, std::wstring fileName) const;
	void CalculateTBN(const struct VertexBasic& v1, const struct VertexBasic& v2, const struct VertexBasic& v3,
		DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);
	void BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void BuildMaterial();

private:
	static inline AssetManager* instance = nullptr;
	static inline bool isInitialized = false;

	std::unordered_map<std::string, std::unique_ptr<class Mesh>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<class Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<struct Texture>> mTextures;
	std::unordered_map<std::string, std::unique_ptr<class Sound>> mSounds;

	// 텍스처의 이름과 경로를 pair로 묶음
	// Material에 사용하기 위한 Index를 위해 vector로 선언한다.
	// 텍스쳐의 수가 변한다면 Common.hlsl의 Texture2D의 배열 크기를 똑같도록 수정한다.
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
	};

	// h3d 모델의 이름과 함께 충돌 타입을 설정한다.
	const std::vector<MeshInfo> mH3dModels =
	{
		{ "Cube_AABB",  L"Models/Cube.h3d", CollisionType::AABB },
		{ "Cube_OBB",  L"Models/Cube.h3d", CollisionType::OBB },
		{ "Sphere",  L"Models/Sphere.h3d", CollisionType::Sphere },
		{ "SkySphere",  L"Models/Sphere.h3d", CollisionType::None },
		{ "Cylinder",  L"Models/Cylinder.h3d", CollisionType::AABB },
	};

	// obj 모델의 이름과 함께 충돌 타입을 설정한다.
	// obj 모델은 임포트함과 동시에 h3d 파일로 변환한다.
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