#include <Framework/AssetLoader.h>
#include <Component/Mesh.h>
#include <fstream>
#include <iostream>

bool AssetLoader::LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::wstring fileName,
	std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
	std::fstream fin;
	char input;

	fin.open(fileName);

	// 파일이 없거나 이름이 잘못된 경우 실패한다.
	if (fin.fail())
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Model h3d " << fileName.c_str() << " File Open Failed" << std::endl;
#endif
		return false;
	}

	UINT vertexCount;
	fin >> vertexCount;
	vertices.reserve(vertexCount);

	for (UINT i = 0; i < vertexCount; ++i)
	{
		// h3d파일에서 Vertex에 관한 정보를 읽어온다.
		Vertex v;
		fin >> v.pos.x >> v.pos.y >> v.pos.z >> input;
		fin >> v.normal.x >> v.normal.y >> v.normal.z >> input;
		fin >> v.tangentU.x >> v.tangentU.y >> v.tangentU.z >> input;
		fin >> v.binormalU.x >> v.binormalU.y >> v.binormalU.z >> input;
		fin >> v.texC.x >> v.texC.y >> input;
		fin >> v.color.x >> v.color.y >> v.color.z >> v.color.w;

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

	return true;
}

bool AssetLoader::ConvertObj(const std::wstring fileName)
{
	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT2> texcoords;
	std::vector<XMFLOAT3> normals;
	std::vector<FaceIndex> faceIndices;
	std::fstream fin;

	fin.open(fileName);

	// 파일이 없거나 이름이 잘못된 경우 실패한다.
	if (fin.fail())
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Model obj " << fileName.c_str() << " File Open Failed" << std::endl;
#endif
		return false;
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
				fin >> f[2].posIndex >> input >> f[2].texIndex >> input >> f[2].normalIndex;
				fin >> f[1].posIndex >> input >> f[1].texIndex >> input >> f[1].normalIndex;
				fin >> f[0].posIndex >> input >> f[0].texIndex >> input >> f[0].normalIndex;

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
	std::vector<UINT16> indices;
	std::vector<FaceIndex> overlapFaceIndices;
	std::vector<UINT16> overlappedVertexNums;
	UINT32 vertexIndex = 0;
	for (UINT32 i = 0; i < (UINT32)faceIndices.size(); i += 3)
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
			v[j].pos = positions.at(f[j].posIndex - 1);
			v[j].normal = normals.at(f[j].normalIndex - 1);
			v[j].texC = texcoords.at(f[j].texIndex - 1);
		}

		// 노멀 매핑 및 라이팅에 사용항 TBN을 계산한다.
		XMFLOAT3 normal, tangent, binormal;
		CalculateTBN(v[0], v[1], v[2], normal, tangent, binormal);

		// vector가 empty일 때 find 함수를 사용하면 incompatible 오류가 생긴다.
		// 첫 번째 삽입의 경우 중복되는 정점이 존재할 수 없으므로 삽입한다.
		if (i == 0)
		{
			for (UINT32 j = 0; j < 3; ++j)
			{
				vertices.emplace_back(v[j].pos, v[j].normal, tangent, binormal, v[j].texC);
				indices.emplace_back(vertexIndex);
				overlapFaceIndices.emplace_back(f[j]);
				overlappedVertexNums.emplace_back(1);
				++vertexIndex;
			}
			continue;
		}

		// obj파일에 기록된 Face 인덱스에는 중복되는 정점 인덱스가 존재한다.
		// 따라서 정점 버퍼를 만들때에는 중복되는 정점을 제거해야 한다.
		for (UINT32 j = 0; j < 3; ++j)
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
				vertices[index].normal = Vector3::Add(vertices[index].normal, v[j].normal);
				vertices[index].tangentU = Vector3::Add(vertices[index].tangentU, tangent); 
				vertices[index].binormalU = Vector3::Add(vertices[index].binormalU, binormal);
			}
			// 아니라면
			else
			{
				// 새로운 정점과 인덱스를 삽입한다.
				vertices.emplace_back(v[j].pos, v[j].normal, tangent, binormal, v[j].texC);
				indices.emplace_back(vertexIndex);
				overlapFaceIndices.emplace_back(f[j]);
				overlappedVertexNums.emplace_back(1);
				++vertexIndex;
			}
		}
	}

	for (UINT32 i = 0; i < (UINT32)overlappedVertexNums.size(); ++i)
	{
		// 중복되는 TBN을 나누어 평균을 구한다.
		auto n = overlappedVertexNums[i];
		vertices[i].normal = Vector3::Divide(vertices[i].normal, n);
		vertices[i].tangentU = Vector3::Divide(vertices[i].tangentU, n);
		vertices[i].binormalU = Vector3::Divide(vertices[i].binormalU, n);
	}
	
	// obj파일은 h3d파일로 변환한다.
	ObjToH3d(vertices, indices, fileName); 

	return true;
}

void AssetLoader::ObjToH3d(const std::vector<Vertex>& vertices, const std::vector<std::uint16_t>& indices, std::wstring fileName)
{
	static const std::wstring fileFormat = L".h3d";
	fileName.erase(fileName.size() - 4, 4); // fileName에서 obj 포맷 string을 지운다.
	fileName += fileFormat; // 파일에 h3d 파일형식을 붙인다.

	std::ofstream fout(fileName);

	fout << vertices.size() << std::endl; // VertexCount
	for (const auto& vertex : vertices)
	{
		// 파일에 Vertex에 관한 정보를 쓴다.
		fout << vertex.pos.x << " " << vertex.pos.y << " " << vertex.pos.z << " / ";
		fout << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << " / ";
		fout << vertex.tangentU.x << " " << vertex.tangentU.y << " " << vertex.tangentU.z << " / ";
		fout << vertex.binormalU.x << " " << vertex.binormalU.y << " " << vertex.binormalU.z << " / ";
		fout << vertex.texC.x << " " << vertex.texC.y << " / ";
		fout << vertex.color.x << " " << vertex.color.y << " " << vertex.color.z << " " << vertex.color.w << std::endl;
	}

	fout << indices.size() << std::endl; // Index Count
	for (const auto& i : indices)
	{
		// 인덱스를 쓴다.
		fout << i << std::endl;
	}

	fout.close();
}

void AssetLoader::CalculateTBN(const VertexBasic& v1, const VertexBasic& v2, const VertexBasic& v3,
	XMFLOAT3& normal, XMFLOAT3& tangent, XMFLOAT3& binormal)
{
	XMVECTOR pv1, pv2;
	XMVECTOR tu1, tv1;

	XMVECTOR p1 = XMLoadFloat3(&v1.pos);
	XMVECTOR p2 = XMLoadFloat3(&v2.pos);
	XMVECTOR p3 = XMLoadFloat3(&v3.pos);
	XMVECTOR t1 = XMLoadFloat2(&v1.texC);
	XMVECTOR t2 = XMLoadFloat2(&v2.texC);
	XMVECTOR t3 = XMLoadFloat2(&v3.texC);

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


FILE* AssetLoader::LoadWave(const std::string fileName, WaveHeaderType& header)
{
	FILE* filePtr = nullptr;
	if (fopen_s(&filePtr, fileName.c_str(), "rb") != 0)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << fileName.c_str() << " Wave File not Found" << std::endl;
#endif
		return nullptr;
	}

	if (fread(&header, sizeof(WaveHeaderType), 1, filePtr) == -1)
		return nullptr;


	// RIFF 포맷인지 확인한다.
	if ((header.chunkId[0] != 'R') || (header.chunkId[1] != 'I') ||
		(header.chunkId[2] != 'F') || (header.chunkId[3] != 'F'))
		return nullptr;

	// Wave 포맷인지 확인한다.
	if ((header.format[0] != 'W') || (header.format[1] != 'A') ||
		(header.format[2] != 'V') || (header.format[3] != 'E'))
		return nullptr;

	// fmt 포맷인지 확인한다.
	if ((header.subChunkId[0] != 'f') || (header.subChunkId[1] != 'm') ||
		(header.subChunkId[2] != 't') || (header.subChunkId[3] != ' '))
		return nullptr;

	/*
	// 오디오 포맷이 WAVE_FORMAT_PCM인지 확인한다.
	if (header.audioFormat != WAVE_FORMAT_PCM)
		return nullptr;

	// 스테레오 포맷으로 기록되어 있는지 확인한다.
	if (header.numChannels != 2)
		return nullptr;

	// 44.1kHz 비율로 기록되었는지 확인한다.
	if (header.sampleRate != 44100)
		return nullptr;

	// 16비트로 기록되어 있는지 확인한다.
	if (header.bitsPerSample != 16)
		return nullptr;
	*/

	// data인지 확인한다.
	if ((header.dataChunkId[0] != 'd') || (header.dataChunkId[1] != 'a') ||
		(header.dataChunkId[2] != 't') || (header.dataChunkId[3] != 'a'))
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << fileName.c_str() << " Not Match Data" << std::endl;
#endif
		return nullptr;
	}

	return filePtr;
}