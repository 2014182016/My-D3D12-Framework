#include "pch.h"
#include "AssetManager.h"
#include "Material.h"
#include "Mesh.h"
#include "D3DUtil.h"
#include "Sound.h"
#include "AssetLoader.h"
#include "DDSTextureLoader.h"

using namespace DirectX;
using namespace std::literals;

AssetManager* AssetManager::GetInstance()
{
	if (instance == nullptr)
		instance = new AssetManager();
	return instance;
}

AssetManager::AssetManager() { }

AssetManager::~AssetManager() 
{
	mMeshes.clear();
	mMaterials.clear();
	mTextures.clear();
	mSounds.clear();

	delete this;
}

void AssetManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, IDirectSound8* d3dSound)
{
	if (isInitialized)
		return;

	LoadTextures(device, cmdList);
	LoadMeshes(device, cmdList);
	LoadSounds(device, cmdList, d3dSound);
	BuildMaterial();

	isInitialized = true;
}

void AssetManager::BuildMaterial()
{
	std::unique_ptr<Material> mat;

	mat = std::make_unique<Material>("Default"s);
	mat->SetDiffuseMapIndex(DISABLED);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughness(0.25f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Brick0"s);
	mat->SetDiffuseMapIndex(FindTexture("Bricks2"s)->mTextureIndex);
	mat->SetNormalMapIndex(FindTexture("Bricks2_nmap"s)->mTextureIndex);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughness(0.9f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Tile0"s);
	mat->SetDiffuseMapIndex(FindTexture("Tile"s)->mTextureIndex);
	mat->SetNormalMapIndex(FindTexture("Tile_nmap"s)->mTextureIndex);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.2f, 0.2f, 0.2f);
	mat->SetRoughness(0.1f);
	mat->SetScale(5.0f, 5.0f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Mirror0"s);
	mat->SetDiffuseMapIndex(FindTexture("Ice"s)->mTextureIndex);
	mat->SetNormalMapIndex(FindTexture("Default_nmap"s)->mTextureIndex);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.98f, 0.97f, 0.95f);
	mat->SetRoughness(0.1f);
	mat->SetOpacity(0.5f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Wirefence"s);
	mat->SetDiffuseMapIndex(FindTexture("WireFence"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughness(0.25f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Tree0"s);
	mat->SetDiffuseMapIndex(FindTexture("Tree01S"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.01f, 0.01f, 0.01f);
	mat->SetRoughness(0.125f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Sky"s);
	mat->SetDiffuseMapIndex(FindTexture("Clouds"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughness(1.0f);
	mat->SetScale(2.0f, 2.0f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Radial_Gradient"s);
	mat->SetDiffuseMapIndex(FindTexture("Radial_Gradient"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughness(1.0f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Terrain"s);
	mat->SetDiffuseMapIndex(FindTexture("Grass"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetHeightMapIndex(FindTexture("HeightMap"s)->mTextureIndex);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughness(0.9f);
	mMaterials[mat->GetName()] = std::move(mat);
}

void AssetManager::LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	for (size_t i = 0; i < mTexInfos.size(); ++i)
	{
		auto[texName, fileName] = mTexInfos[i];

		auto texMap = std::make_unique<Texture>();
		texMap->mName = texName;
		texMap->mFilename = fileName;
		texMap->mTextureIndex = (std::uint32_t)i;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device, commandList,
			texMap->mFilename.c_str(), texMap->mResource, texMap->mUploadHeap));

		mTextures[texName] = std::move(texMap);
	}

	if ((UINT)mTextures.size() != TEX_NUM)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Max Tex Num Difference Error" << std::endl;
#endif
	}
}

void AssetManager::LoadSounds(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, IDirectSound8* d3dSound)
{
	for (const auto& soundInfo : mSoundInfos)
	{
		WaveHeaderType header;

		FILE* filePtr = AssetLoader::LoadWave(d3dSound, soundInfo.mFileName, header);
		if (filePtr == nullptr)
			continue;

		auto soundName = soundInfo.mName;
		std::unique_ptr<Sound> sound = std::make_unique<Sound>(std::move(soundName));

		switch (soundInfo.mSoundType)
		{
		case SoundType::Sound2D:
			sound->CreateSoundBuffer2D(d3dSound, filePtr, header);
			break;
		case SoundType::Sound3D:
			sound->CreateSoundBuffer3D(d3dSound, filePtr, header);
			break;
		}

		mSounds[soundInfo.mName] = std::move(sound);
	}
}

void AssetManager::LoadMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	for (const auto& h3dInfo : mH3dModels)
	{
		std::vector<Vertex> vertices;
		std::vector<std::uint16_t> indices;

		bool result = AssetLoader::LoadH3d(device, commandList, h3dInfo.mFileName, vertices, indices);
		if (!result)
			continue;

		auto meshName = h3dInfo.mName;
		std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(std::move(meshName));

		mesh->BuildVertices(device, commandList, (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(Vertex));
		mesh->BuildIndices(device, commandList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));
		mesh->BuildCollisionBound(&vertices[0].mPos, vertices.size(), (size_t)sizeof(Vertex), h3dInfo.mCollisionType);

		mMeshes[h3dInfo.mName] = std::move(mesh);
	}
}

Mesh* AssetManager::FindMesh(std::string&& name) const
{
	auto iter = mMeshes.find(name);

	if (iter == mMeshes.end())
	{
#if defined(DEBUG) | defined(_DEBUG)
		std::cout << "Geometry " << name << "이 발견되지 않음." << std::endl;
#endif
		return nullptr;
	}

	return (*iter).second.get();
}

Material* AssetManager::FindMaterial(std::string&& name) const
{
	auto iter = mMaterials.find(name);

	if (iter == mMaterials.end())
	{
#if defined(DEBUG) | defined(_DEBUG)
		std::cout << "Material " << name << "이 발견되지 않음." << std::endl;
#endif
		return nullptr;
	}

	return (*iter).second.get();
}

Sound* AssetManager::FindSound(std::string&& name) const
{
	auto iter = mSounds.find(name);

	if (iter == mSounds.end())
	{
#if defined(DEBUG) | defined(_DEBUG)
		std::cout << "Sound " << name << "이 발견되지 않음." << std::endl;
#endif
		return nullptr;
	}

	return (*iter).second.get();
}

Texture* AssetManager::FindTexture(std::string&& name) const
{
	auto iter = mTextures.find(name);

	if (iter == mTextures.end())
	{
#if defined(DEBUG) | defined(_DEBUG)
		std::cout << "Texture " << name << "이 발견되지 않음." << std::endl;
#endif
		return nullptr;
	}

	return (*iter).second.get();
}


ID3D12Resource* AssetManager::GetTextureResource(UINT index) const
{
	std::string texName = mTexInfos[index].mName;
	auto tex = mTextures.find(texName);

	return tex->second->mResource.Get();
}