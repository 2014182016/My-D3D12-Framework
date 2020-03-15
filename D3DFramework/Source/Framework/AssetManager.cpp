#include "pch.h"
#include "AssetManager.h"
#include "Material.h"
#include "Mesh.h"
#include "D3DUtil.h"
#include "Structure.h"
#include "Sound.h"

using namespace DirectX;
using namespace std::literals;
using TexName = std::pair<std::string, std::wstring>;

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
}

void AssetManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, IDirectSound8* d3dSound)
{
	if (isInitialized)
		return;

	LoadTextures(device, commandList);
	LoadWaves(d3dSound);
	BuildGeometry(device, commandList);
	BuildMaterial();

	isInitialized = true;
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

void AssetManager::LoadWaves(IDirectSound8* d3dSound)
{
	for (const auto& soundInfo : mSoundInfos)
		LoadWave(d3dSound, soundInfo);
}

void AssetManager::LoadObj(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshInfo& modelInfo)
{
	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT2> texcoords;
	std::vector<XMFLOAT3> normals;
	std::vector<FaceIndex> faceIndices;
	std::fstream fin;
	
	auto[meshName, fileName, collisionType] = modelInfo;

	fin.open(fileName);
	if (fin.fail())
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Model obj " << fileName.c_str() << " File Open Failed" << std::endl;
#endif
		return;
	}

	char input;
	do
	{
		fin.get(input);

		if (input == 'v')
		{
			fin.get(input);

			// Vertex Position
			if (input == ' ')
			{
				XMFLOAT3 pos;
				fin >> pos.x >> pos.y >> pos.z;
				pos.z *= -1.0f; // 오른손 좌표계를 왼손 좌표계로 변환
				positions.emplace_back(pos);
			}
			// Vertex Texcoord
			else if (input == 't')
			{
				XMFLOAT2 texcoord;
				fin >> texcoord.x >> texcoord.y;
				texcoord.y = 1.0f - texcoord.y; // 텍스처 좌표를 뒤집는다.
				texcoords.emplace_back(texcoord);
			}
			// Vertex Normal
			else if (input == 'n')
			{
				XMFLOAT3 normal;
				fin >> normal.x >> normal.y >> normal.z;
				normal.z *= -1.0f;  // 오른손 좌표계를 왼손 좌표계로 변환
				normals.emplace_back(normal);
			}
		}
		// Face
		else if (input == 'f')
		{
			fin.get(input);

			if (input == ' ') 
			{
				FaceIndex f[3];
				// 반시계 방향으로 정의된 평면을 시계 방향으로 바꾸기 위해 거꾸로 읽는다.
				fin >> f[2].mPosIndex >> input >> f[2].mTexIndex >> input >> f[2].mNormalIndex;
				fin >> f[1].mPosIndex >> input >> f[1].mTexIndex >> input >> f[1].mNormalIndex;
				fin >> f[0].mPosIndex >> input >> f[0].mTexIndex >> input >> f[0].mNormalIndex;

				faceIndices.emplace_back(f[0]);
				faceIndices.emplace_back(f[1]);
				faceIndices.emplace_back(f[2]);
			}
		}

		// 나머지 행을 읽는다.
		while (input != '\n')
			fin.get(input);

	} while (!fin.eof());

	fin.close();

	std::vector<Vertex> vertices;
	std::vector<std::uint16_t> indices;
	std::vector<FaceIndex> overlapFaceIndices;
	std::vector<std::uint16_t> overlappedVertexNums;
	UINT vertexIndex = 0;
	for (UINT i = 0; i < (UINT)faceIndices.size(); i += 3)
	{
		VertexBasic v[3];
		FaceIndex f[3];

		f[0] = faceIndices[i];
		f[1] = faceIndices[i + 1];
		f[2] = faceIndices[i + 2];

		// 한 면(Face)에 사용되는 각 데이터의 인덱스에 따라 정보를 불러온다.
		for (UINT j = 0; j < 3; ++j)
		{
			// obj파일의 인덱스는 1부터 시작하므로 -1을 더해준다.
			v[j].mPos = positions.at(f[j].mPosIndex - 1);
			v[j].mNormal = normals.at(f[j].mNormalIndex - 1);
			v[j].mTexC = texcoords.at(f[j].mTexIndex - 1);
		}

		// 노멀 매핑 및 라이팅에 사용항 TBN을 계산한다.
		XMFLOAT3 normal, tangent, binormal;
		CalculateTBN(v[0], v[1], v[2], normal, tangent, binormal);

		// vector가 empty일 때 find 함수를 사용하면 incompatible 오류가 생긴다.
		// 첫 번째 삽입의 경우 중복되는 정점이 존재할 수 없으므로 삽입한다.
		if (i == 0)
		{
			for (UINT j = 0; j < 3; ++j)
			{
				vertices.emplace_back(v[j].mPos, v[j].mNormal, tangent, binormal, v[j].mTexC);
				indices.emplace_back(vertexIndex);
				overlapFaceIndices.emplace_back(f[j]);
				overlappedVertexNums.emplace_back(1);
				++vertexIndex;
			}
			continue;
		}

		// obj파일에 기록된 Face 인덱스에는 중복되는 정점 인덱스가 존재한다.
		// 따라서 정점 버퍼를 만들때에는 중복되는 정점을 제거해야 한다.
		for (UINT j = 0; j < 3; ++j)
		{
			auto iter = std::find(overlapFaceIndices.begin(), overlapFaceIndices.end(), f[j]);
			// 만약 겹치는 FaceIndex가 존재한다면
			if (iter != overlapFaceIndices.end())
			{
				// 중복되는 정점의 인덱스를 삽입한다.
				UINT index = (UINT)std::distance(overlapFaceIndices.begin(), iter);
				++overlappedVertexNums[index];
				indices.emplace_back(index);
				// 중복되는 TBN을 더한다.
				vertices[index].mNormal = vertices[index].mNormal + v[j].mNormal;
				vertices[index].mTangentU = vertices[index].mTangentU + tangent;
				vertices[index].mBinormalU = vertices[index].mBinormalU  + binormal;
			}
			// 아니라면
			else
			{
				// 새로운 정점과 인덱스를 삽입한다.
				vertices.emplace_back(v[j].mPos, v[j].mNormal, tangent, binormal, v[j].mTexC);
				indices.emplace_back(vertexIndex);
				overlapFaceIndices.emplace_back(f[j]);
				overlappedVertexNums.emplace_back(1);
				++vertexIndex;
			}
		}
	}

	for (size_t i = 0; i < overlappedVertexNums.size(); ++i)
	{
		// 중복되는 TBN을 나누어 평균을 구한다.
		auto n = overlappedVertexNums[i];
		vertices[i].mNormal = vertices[i].mNormal / n;
		vertices[i].mTangentU = vertices[i].mTangentU / n;
		vertices[i].mBinormalU = vertices[i].mBinormalU / n;
	}

	fileName.erase(fileName.size() - 4, 4); // fileName에서 obj 포맷 string을 지운다.
	ObjToH3d(vertices, indices, fileName); // obj파일은 h3d파일로 변환한다.

	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(std::move(meshName));
	mesh->BuildVertices(device, commandList, (void*)vertices.data(),(UINT)vertices.size(), (UINT)sizeof(Vertex));
	mesh->BuildIndices(device, commandList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));

	if(collisionType != CollisionType::None)
		mesh->BuildCollisionBound(&vertices[0].mPos, vertices.size(), (size_t)sizeof(Vertex), collisionType);

	mMeshes[mesh->GetName()] = std::move(mesh);
}

void AssetManager::LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshInfo& modelInfo)
{
	std::vector<Vertex> vertices;
	std::vector<std::uint16_t> indices;
	std::fstream fin;
	char input;

	auto[meshName, fileName, collisionType] = modelInfo;

	fin.open(fileName);
	if (fin.fail())
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Model h3d " << fileName.c_str() << " File Open Failed" << std::endl;
#endif
		return;
	}

	UINT vertexCount;
	fin >> vertexCount;
	vertices.reserve(vertexCount);
	for (UINT i = 0; i < vertexCount; ++i)
	{
		// h3d파일에서 Vertex에 관한 정보를 읽어온다.
		Vertex v;
		fin >> v.mPos.x >> v.mPos.y >> v.mPos.z >> input;
		fin >> v.mNormal.x >> v.mNormal.y >> v.mNormal.z >> input;
		fin >> v.mTangentU.x >> v.mTangentU.y >> v.mTangentU.z >> input;
		fin >> v.mBinormalU.x >> v.mBinormalU.y >> v.mBinormalU.z >> input;
		fin >> v.mTexC.x >> v.mTexC.y >> input;
		fin >> v.mColor.x >> v.mColor.y >> v.mColor.z >> v.mColor.w;

		vertices.emplace_back(v);
	}

	UINT indexCount;
	fin >> indexCount;
	indices.reserve(indexCount);
	for (UINT i = 0; i < indexCount; ++i)
	{
		// 인덱스 정보를 읽어온다.
		std::uint16_t index;
		fin >> index;
		indices.emplace_back(index);
	}

	fin.close();

	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(std::move(meshName));
	mesh->BuildVertices(device, commandList, (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(Vertex));
	mesh->BuildIndices(device, commandList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));

	if (collisionType != CollisionType::None)
		mesh->BuildCollisionBound(&vertices[0].mPos, vertices.size(), (size_t)sizeof(Vertex), collisionType);

	mMeshes[mesh->GetName()] = std::move(mesh);
}

void AssetManager::ObjToH3d(const std::vector<struct Vertex>& vertices, const std::vector<std::uint16_t> indices, std::wstring fileName) const
{
	std::wstring fileFormat = L".h3d";
	fileName += fileFormat;

	std::ofstream fout(fileName);

	fout << vertices.size() << std::endl; // VertexCount
	for (const auto& vertex : vertices)
	{
		// 파일에 Vertex에 관한 정보를 쓴다.
		fout << vertex.mPos.x << " " << vertex.mPos.y << " " << vertex.mPos.z << " / ";
		fout << vertex.mNormal.x << " " << vertex.mNormal.y << " " << vertex.mNormal.z << " / ";
		fout << vertex.mTangentU.x << " " << vertex.mTangentU.y << " " << vertex.mTangentU.z << " / ";
		fout << vertex.mBinormalU.x << " " << vertex.mBinormalU.y << " " << vertex.mBinormalU.z << " / ";
		fout << vertex.mTexC.x << " " << vertex.mTexC.y << " / ";
		fout << vertex.mColor.x << " " << vertex.mColor.y << " " << vertex.mColor.z << " " << vertex.mColor.w << std::endl;
	}

	fout << indices.size() << std::endl; // Index Count
	for (const auto& i : indices)
	{
		// 인덱스를 쓴다.
		fout << i << std::endl;
	}

	fout.close();
}

void AssetManager::CalculateTBN(const VertexBasic& v1, const VertexBasic& v2, const VertexBasic& v3, 
	XMFLOAT3& normal, XMFLOAT3& tangent, XMFLOAT3& binormal)
{
	XMVECTOR pv1, pv2;
	XMVECTOR tu1, tv1;

	XMVECTOR p1 = XMLoadFloat3(&v1.mPos);
	XMVECTOR p2 = XMLoadFloat3(&v2.mPos);
	XMVECTOR p3 = XMLoadFloat3(&v3.mPos);
	XMVECTOR t1 = XMLoadFloat2(&v1.mTexC);
	XMVECTOR t2 = XMLoadFloat2(&v2.mTexC);
	XMVECTOR t3 = XMLoadFloat2(&v3.mTexC);

	// 현재 표면의 두 벡터를 계산한다.
	pv1 = p2 - p1;
	pv2 = p3 - p1;

	// tu 및 tv 텍스처 공간 벡터를 계산한다.
	tu1 = t2 - t1;
	tv1 = t3 - t1;

	XMFLOAT3 tu, tv;
	XMStoreFloat3(&tu, tu1);
	XMStoreFloat3(&tv, tv1);

	// 탄젠트 / 바이 노멀 방정식의 분모를 계산한다.
	float denominator = (tu.x * tv.y - tu.y * tv.x);
	if (denominator < FLT_EPSILON)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "CalculateTBN Arithmetic Error" << std::endl;
#endif
		normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		tangent = XMFLOAT3(0.0f, 0.0f, 0.0f);
		binormal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		return;
	}
	float den = 1.0f / denominator;


	// 교차 곱을 계산하고 계수로 곱하여 접선과 비 구식을 얻는다.
	XMVECTOR tangentVec = (tv.y * pv1 - tv.x * pv2) * den;
	tangentVec = XMVector3Normalize(tangentVec);
	XMVECTOR binormalVec = (tu.x * pv2 - tu.y * pv1) * den;
	binormalVec = XMVector3Normalize(binormalVec);
	XMVECTOR normalVec = XMVector3Cross(tangentVec, binormalVec);
	normalVec = XMVector3Normalize(normalVec);

	XMStoreFloat3(&normal, normalVec);
	XMStoreFloat3(&tangent, tangentVec);
	XMStoreFloat3(&binormal, binormalVec);
}

void AssetManager::LoadWave(IDirectSound8* d3dSound, const SoundInfo& soundInfo)
{
	auto[objectName, fileName, soundType] = soundInfo;

	FILE* filePtr = nullptr;
	if(fopen_s(&filePtr, fileName.c_str(), "rb") != 0)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << fileName.c_str() << " Wave File not Found" << std::endl;
#endif
		return;
	}

	WaveHeaderType waveFileHeader;
	if (fread(&waveFileHeader, sizeof(waveFileHeader), 1, filePtr) == -1)
		return;


	// RIFF 포맷인지 확인한다.
	if ((waveFileHeader.chunkId[0] != 'R') || (waveFileHeader.chunkId[1] != 'I') ||
		(waveFileHeader.chunkId[2] != 'F') || (waveFileHeader.chunkId[3] != 'F'))
		return;

	// Wave 포맷인지 확인한다.
	if ((waveFileHeader.format[0] != 'W') || (waveFileHeader.format[1] != 'A') ||
		(waveFileHeader.format[2] != 'V') || (waveFileHeader.format[3] != 'E'))
		return;

	// fmt 포맷인지 확인한다.
	if ((waveFileHeader.subChunkId[0] != 'f') || (waveFileHeader.subChunkId[1] != 'm') ||
		(waveFileHeader.subChunkId[2] != 't') || (waveFileHeader.subChunkId[3] != ' '))
		return;

	/*
	// 오디오 포맷이 WAVE_FORMAT_PCM인지 확인한다.
	if (waveFileHeader.audioFormat != WAVE_FORMAT_PCM)
		return;

	// 스테레오 포맷으로 기록되어 있는지 확인한다.
	if (waveFileHeader.numChannels != 2)
		return;

	// 44.1kHz 비율로 기록되었는지 확인한다.
	if (waveFileHeader.sampleRate != 44100)
		return;

	// 16비트로 기록되어 있는지 확인한다.
	if (waveFileHeader.bitsPerSample != 16)
		return;
	*/

	// data인지 확인한다.
	if ((waveFileHeader.dataChunkId[0] != 'd') || (waveFileHeader.dataChunkId[1] != 'a') ||
		(waveFileHeader.dataChunkId[2] != 't') || (waveFileHeader.dataChunkId[3] != 'a'))
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << fileName.c_str() << " Not Match Data" << std::endl;
#endif
		return;
	}

	std::unique_ptr<Sound> sound = std::make_unique<Sound>(std::move(objectName));

	switch (soundType)
	{
	case SoundType::Sound2D:
		sound->CreateSoundBuffer2D(d3dSound, filePtr, waveFileHeader);
		break;
	case SoundType::Sound3D:
		sound->CreateSoundBuffer3D(d3dSound, filePtr, waveFileHeader);
		break;
	}

	mSounds[sound->GetName()] = std::move(sound);
}

void AssetManager::BuildMaterial()
{
	std::unique_ptr<Material> mat;

	mat = std::make_unique<Material>("Default"s);
	mat->SetDiffuseMapIndex(DISABLED);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughtness(0.25f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Brick0"s);
	mat->SetDiffuseMapIndex(FindTexture("Bricks2"s)->mTextureIndex);
	mat->SetNormalMapIndex(FindTexture("Bricks2_nmap"s)->mTextureIndex);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughtness(0.9f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Tile0"s);
	mat->SetDiffuseMapIndex(FindTexture("Tile"s)->mTextureIndex);
	mat->SetNormalMapIndex(FindTexture("Tile_nmap"s)->mTextureIndex);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.2f, 0.2f, 0.2f);
	mat->SetRoughtness(0.3f);
	mat->SetScale(5.0f, 5.0f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Mirror0"s);
	mat->SetDiffuseMapIndex(FindTexture("Ice"s)->mTextureIndex);
	mat->SetNormalMapIndex(FindTexture("Default_nmap"s)->mTextureIndex);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.98f, 0.97f, 0.95f);
	mat->SetRoughtness(0.1f);
	mat->SetOpacity(0.5f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Wirefence"s);
	mat->SetDiffuseMapIndex(FindTexture("WireFence"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughtness(0.25f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Tree0"s);
	mat->SetDiffuseMapIndex(FindTexture("Tree01S"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.01f, 0.01f, 0.01f);
	mat->SetRoughtness(0.125f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Sky"s);
	mat->SetDiffuseMapIndex(FindTexture("Clouds"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughtness(1.0f);
	mat->SetScale(2.0f, 2.0f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Radial_Gradient"s);
	mat->SetDiffuseMapIndex(FindTexture("Radial_Gradient"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughtness(1.0f);
	mMaterials[mat->GetName()] = std::move(mat);

	mat = std::make_unique<Material>("Terrain"s);
	mat->SetDiffuseMapIndex(FindTexture("Grass"s)->mTextureIndex);
	mat->SetNormalMapIndex(DISABLED);
	mat->SetHeightMapIndex(FindTexture("HeightMap"s)->mTextureIndex);
	mat->SetDiffuse(1.0f, 1.0f, 1.0f);
	mat->SetSpecular(0.1f, 0.1f, 0.1f);
	mat->SetRoughtness(0.9f);
	mMaterials[mat->GetName()] = std::move(mat);
}

void AssetManager::BuildGeometry(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	for (const auto& h3dInfo : mH3dModels)
		LoadH3d(device, commandList, h3dInfo);

	for (const auto& objInfo : mObjModels)
		LoadObj(device, commandList, objInfo);
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