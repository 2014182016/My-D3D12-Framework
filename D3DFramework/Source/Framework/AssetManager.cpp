#include "DDSTextureLoader.h"
#include <Framework/AssetManager.h>
#include <Framework/DDSTextureLoader.h>
#include <Component/Material.h>
#include <Component/Mesh.h>
#include <Component/Sound.h>
#include <iostream>

using namespace std::literals;

static const std::wstring texturePath = L"../Assets/Textures/";
static const std::wstring modelPath = L"../Assets/Models/";
static const std::string soundPath = "../Assets/Sounds/";

AssetManager* AssetManager::GetInstance()
{
	static AssetManager* instance = nullptr;
	if (instance == nullptr)
		instance = new AssetManager();
	return instance;
}

AssetManager::AssetManager() { }

AssetManager::~AssetManager() 
{
	meshes.clear();
	materials.clear();
	textures.clear();
	sounds.clear();
}

void AssetManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, IDirectSound8* d3dSound)
{
	// 애셋 매니저는 싱글턴 패턴이기 때문에
	// 단 한번만 초기화가 가능한다.
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
	mat->diffuseMapIndex = DISABLED;
	mat->normalMapIndex = DISABLED;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.1f, 0.1f, 0.1f };
	mat->roughness = 0.25f;
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Brick0"s);
	mat->diffuseMapIndex = FindTexture("Bricks2"s)->textureIndex;
	mat->normalMapIndex = FindTexture("Bricks2_nmap"s)->textureIndex;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.1f, 0.1f, 0.1f };
	mat->roughness = 0.9f;
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Tile0"s);
	mat->diffuseMapIndex = FindTexture("Tile"s)->textureIndex;
	mat->normalMapIndex = FindTexture("Tile_nmap"s)->textureIndex;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.2f, 0.2f, 0.2f };
	mat->roughness = 0.1f;
	mat->SetScale(5.0f, 5.0f);
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Mirror0"s);
	mat->diffuseMapIndex = FindTexture("Ice"s)->textureIndex;
	mat->normalMapIndex = FindTexture("Default_nmap"s)->textureIndex;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.98f, 0.97f, 0.95f };
	mat->roughness = 0.1f;
	mat->SetOpacity(0.5f);
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Wirefence"s);
	mat->diffuseMapIndex = FindTexture("WireFence"s)->textureIndex;
	mat->normalMapIndex = DISABLED;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.1f, 0.1f, 0.1f };
	mat->roughness = 0.25f;
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Tree0"s);
	mat->diffuseMapIndex = FindTexture("Tree01S"s)->textureIndex;
	mat->normalMapIndex = DISABLED;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.1f, 0.1f, 0.1f };
	mat->roughness = 0.125f;
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Sky"s);
	mat->diffuseMapIndex = FindTexture("Clouds"s)->textureIndex;
	mat->normalMapIndex = DISABLED;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.1f, 0.1f, 0.1f };
	mat->roughness = 1.0f;
	mat->SetScale(2.0f, 2.0f);
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Radial_Gradient"s);
	mat->diffuseMapIndex = FindTexture("Radial_Gradient"s)->textureIndex;
	mat->normalMapIndex = DISABLED;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.1f, 0.1f, 0.1f };
	mat->roughness = 1.0f;
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Terrain"s);
	mat->diffuseMapIndex = FindTexture("Grass"s)->textureIndex;
	mat->normalMapIndex = DISABLED;
	mat->heightMapIndex = FindTexture("HeightMap"s)->textureIndex;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.1f, 0.1f, 0.1f };
	mat->roughness = 0.9f;
	materials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Skull"s);
	mat->diffuseMapIndex = DISABLED;
	mat->normalMapIndex = DISABLED;
	mat->diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	mat->specular = { 0.1f, 0.1f, 0.1f };
	mat->roughness = 0.99f;
	materials[mat->GetName()] = std::move(mat);
}

void AssetManager::LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	for (size_t i = 0; i < mTexInfos.size(); ++i)
	{
		auto[texName, fileName] = mTexInfos[i];
		const std::wstring filePath = texturePath + fileName;

		auto texMap = std::make_unique<Texture>();
		texMap->name = texName;
		texMap->filename = fileName;
		texMap->textureIndex = (std::uint32_t)i;
		// 텍스처를 로드한다.
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device, commandList,
			filePath.c_str(), texMap->resource, texMap->uploadHeap));

		textures[texName] = std::move(texMap);
	}

	// 텍스처의 수와 쉐이더에 정의된 수가 일치하는 지 확인한다.
	if ((UINT)textures.size() != TEX_NUM)
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
		const std::string filePath = soundPath + soundInfo.fileName;
		WaveHeaderType header;

		// wav파일을 로드한다.
		FILE* filePtr = AssetLoader::LoadWave(filePath, header);
		if (filePtr == nullptr)
			continue;

		// 사운드 객체를 생성한다.
		auto soundName = soundInfo.name;
		std::unique_ptr<Sound> sound = std::make_unique<Sound>(std::move(soundName));

		// 사운드 타입에 따라 사운드 버퍼를 생성한다.
		switch (soundInfo.soundType)
		{
		case SoundType::Sound2D:
			sound->CreateSoundBuffer2D(d3dSound, filePtr, header);
			break;
		case SoundType::Sound3D:
			sound->CreateSoundBuffer3D(d3dSound, filePtr, header);
			break;
		}

		sounds[soundInfo.name] = std::move(sound);
	}
}

void AssetManager::LoadMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	for (const auto& h3dInfo : mH3dModels)
	{
		const std::wstring filePath = modelPath + h3dInfo.fileName;
		std::vector<Vertex> vertices;
		std::vector<std::uint16_t> indices;

		// h3d파일을 로드한다.
		bool result = AssetLoader::LoadH3d(device, commandList, filePath, vertices, indices);
		if (!result)
			continue;

		// 메쉬 객체를 생성한다.
		auto meshName = h3dInfo.name;
		std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(std::move(meshName));

		// 정점 및 인덱스를 gpu에 옮기고, 충돌 바운드를 생성한다.
		mesh->BuildVertices(device, commandList, (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(Vertex));
		mesh->BuildIndices(device, commandList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));
		mesh->BuildCollisionBound(&vertices[0].pos, (UINT32)vertices.size(), (UINT32)sizeof(Vertex), h3dInfo.collisionType);

		meshes[h3dInfo.name] = std::move(mesh);
	}
}

Mesh* AssetManager::FindMesh(std::string&& name) const
{
	auto iter = meshes.find(name);

	if (iter == meshes.end())
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
	auto iter = materials.find(name);

	if (iter == materials.end())
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
	auto iter = sounds.find(name);

	if (iter == sounds.end())
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
	auto iter = textures.find(name);

	if (iter == textures.end())
	{
#if defined(DEBUG) | defined(_DEBUG)
		std::cout << "Texture " << name << "이 발견되지 않음." << std::endl;
#endif
		return nullptr;
	}

	return (*iter).second.get();
}


ID3D12Resource* AssetManager::GetTextureResource(const UINT32 index) const
{
	if (mTexInfos.size() <= index)
	{
#if defined(DEBUG) | defined(_DEBUG)
		std::cout << "TextureResource " << index << "가 초과됨" << std::endl;
#endif
	}

	std::string texName = mTexInfos[index].name;
	auto tex = textures.find(texName);

	return tex->second->resource.Get();
}

void AssetManager::DisposeUploaders()
{
	for (auto& mesh : meshes)
	{
		mesh.second->DisposeUploaders();
	}
}