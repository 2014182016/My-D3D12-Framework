#include "pch.h"
#include "AssetManager.h"

using namespace DirectX;
using TexName = std::pair<std::string, std::wstring>;


AssetManager::AssetManager() { }

AssetManager::~AssetManager() { }

void AssetManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	LoadTextures(device, commandList);
	BuildGeometry(device, commandList);
	BuildMaterial();
}

void AssetManager::LoadTextures(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	for (const auto& texInfo : texNames)
	{
		auto[texName, fileName] = texInfo;

		auto texMap = std::make_unique<Texture>();
		texMap->mName = texName;
		texMap->mFilename = fileName;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device, commandList,
			texMap->mFilename.c_str(), texMap->mResource, texMap->mUploadHeap));

		mTextures[texName] = std::move(texMap);
	}
}

void AssetManager::BuildMaterial()
{
	std::unique_ptr<Material> mat = nullptr;

	mat = std::make_unique<Material>("Default");
	mat->mDiffuseSrvHeapIndex = -1;
	mat->mNormalSrvHeapIndex = -1;
	mat->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mat->mSpecular = XMFLOAT3(0.1f, 0.1f, 0.1f);
	mat->mRoughness = 0.25f;
	mMaterials[mat->mName] = std::move(mat);

	mat = std::make_unique<Material>("Brick0");
	mat->mDiffuseSrvHeapIndex = FindTextureIndex("Bricks2");
	mat->mNormalSrvHeapIndex = FindTextureIndex("Bricks2_nmap");
	mat->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mat->mSpecular = XMFLOAT3(0.1f, 0.1f, 0.1f);
	mat->mRoughness = 0.25f;
	mMaterials[mat->mName] = std::move(mat);

	mat = std::make_unique<Material>("Tile0");
	mat->mDiffuseSrvHeapIndex = FindTextureIndex("Tile");
	mat->mNormalSrvHeapIndex = FindTextureIndex("Tile_nmap");
	mat->mDiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	mat->mSpecular = XMFLOAT3(0.2f, 0.2f, 0.2f);
	mat->mRoughness = 0.1f;
	mMaterials[mat->mName] = std::move(mat);

	mat = std::make_unique<Material>("Mirror0");
	mat->mDiffuseSrvHeapIndex = FindTextureIndex("Ice");
	mat->mNormalSrvHeapIndex = FindTextureIndex("Default_nmap");
	mat->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	mat->mSpecular = XMFLOAT3(0.08f, 0.08f, 0.08f);
	mat->mRoughness = 0.1f;
	mMaterials[mat->mName] = std::move(mat);

	mat = std::make_unique<Material>("Wirefence");
	mat->mDiffuseSrvHeapIndex = FindTextureIndex("WireFence");
	mat->mNormalSrvHeapIndex = -1;
	mat->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mat->mSpecular = XMFLOAT3(0.1f, 0.1f, 0.1f);
	mat->mRoughness = 0.25f;
	mMaterials[mat->mName] = std::move(mat);

	mat = std::make_unique<Material>("Tree0");
	mat->mDiffuseSrvHeapIndex = FindTextureIndex("Tree01S");
	mat->mNormalSrvHeapIndex = -1;
	mat->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mat->mSpecular = XMFLOAT3(0.01f, 0.01f, 0.01f);
	mat->mRoughness = 0.125f;
	mMaterials[mat->mName] = std::move(mat);
}

void AssetManager::BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	GeometryGenerator::MeshData meshData;
	std::unique_ptr<MeshGeometry> mesh;
	std::vector<Vertex> vertices;
	std::vector<std::uint16_t> indices;

	meshData = GeometryGenerator::CreateBox(1.0f, 1.0f, 1.0f, 3);
	mesh = std::make_unique<MeshGeometry>("Box");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position, ver.Normal, ver.TexC, ver.TangentU);
	mesh->BuildMesh(device, commandList, vertices, meshData.GetIndices16());
	mMeshes[mesh->mName] = std::move(mesh);
;	vertices.clear();
	indices.clear();

	meshData = GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40);
	mesh = std::make_unique<MeshGeometry>("Grid");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position, ver.Normal, ver.TexC, ver.TangentU);
	mesh->BuildMesh(device, commandList, vertices, meshData.GetIndices16());
	mMeshes[mesh->mName] = std::move(mesh);
	vertices.clear();
	indices.clear();

	meshData = GeometryGenerator::CreateSphere(0.5f, 20, 20);
	mesh = std::make_unique<MeshGeometry>("Sphere");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position, ver.Normal, ver.TexC, ver.TangentU);
	mesh->BuildMesh(device, commandList, vertices, meshData.GetIndices16());
	mMeshes[mesh->mName] = std::move(mesh);
	vertices.clear();
	indices.clear();

	meshData = GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	mesh = std::make_unique<MeshGeometry>("Cylinder");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position, ver.Normal, ver.TexC, ver.TangentU);
	mesh->BuildMesh(device, commandList, vertices, meshData.GetIndices16());
	mMeshes[mesh->mName] = std::move(mesh);
	vertices.clear();
	indices.clear();

	meshData = GeometryGenerator::CreateQuad(0.0f, 1.0f, 1.0f, 0.0f, 0.0f);
	mesh = std::make_unique<MeshGeometry>("Quad");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position, ver.Normal, ver.TexC, ver.TangentU);
	mesh->BuildMesh(device, commandList, vertices, meshData.GetIndices16());
	mMeshes[mesh->mName] = std::move(mesh);
	vertices.clear();
	indices.clear();
}

MeshGeometry* AssetManager::FindMesh(std::string name) const
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

Material* AssetManager::FindMaterial(std::string name) const
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

Texture* AssetManager::FindTexture(std::string name) const
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
	std::string texName = texNames[index].first;
	auto tex = mTextures.find(texName);

	return tex->second->mResource.Get();
}

int AssetManager::FindTextureIndex(std::string name) const
{
	// texNames가 정렬되어 있지 않은 상태에서 텍스처 이름과 name이 같은 것을 찾는다.
	auto iter = std::find_if(texNames.begin(), texNames.end(),
		[&name](const TexName& pair) -> bool {
		return std::equal(name.begin(), name.end(), pair.first.begin(), pair.first.end()); });


	if (iter == texNames.end())
	{
#if defined(DEBUG) | defined(_DEBUG)
		std::cout << name << "이 발견되지 않음." << std::endl;
#endif
		return -1;
	}

	// texName의 처음과 찾은 지점사이의 거리를 측정하여 index를 넘겨준다.
	return (int)std::distance(texNames.begin(), iter);
}