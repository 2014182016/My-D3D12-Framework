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

	// ������ ���ų� �̸��� �߸��� ��� �����Ѵ�.
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
		// h3d���Ͽ��� Vertex�� ���� ������ �о�´�.
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
		// �ε��� ������ �о�´�.
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

	// ������ ���ų� �̸��� �߸��� ��� �����Ѵ�.
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
				pos.z *= -1.0f; // ������ ��ǥ�踦 �޼� ��ǥ��� ��ȯ
				positions.emplace_back(pos);
			}
			// Vertex Texcoord
			else if (input == 't')
			{
				XMFLOAT2 texcoord;
				fin >> texcoord.x >> texcoord.y;
				texcoord.y = 1.0f - texcoord.y; // �ؽ�ó ��ǥ�� �����´�.
				texcoords.emplace_back(texcoord);
			}
			// Vertex Normal
			else if (input == 'n')
			{
				XMFLOAT3 normal;
				fin >> normal.x >> normal.y >> normal.z;
				normal.z *= -1.0f;  // ������ ��ǥ�踦 �޼� ��ǥ��� ��ȯ
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
				// �ݽð� �������� ���ǵ� ����� �ð� �������� �ٲٱ� ���� �Ųٷ� �д´�.
				fin >> f[2].posIndex >> input >> f[2].texIndex >> input >> f[2].normalIndex;
				fin >> f[1].posIndex >> input >> f[1].texIndex >> input >> f[1].normalIndex;
				fin >> f[0].posIndex >> input >> f[0].texIndex >> input >> f[0].normalIndex;

				faceIndices.emplace_back(f[0]);
				faceIndices.emplace_back(f[1]);
				faceIndices.emplace_back(f[2]);
			}
		}


		// ������ ���� �д´�.
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

		// �� ��(Face)�� ���Ǵ� �� �������� �ε����� ���� ������ �ҷ��´�.
		for (UINT j = 0; j < 3; ++j)
		{
			// obj������ �ε����� 1���� �����ϹǷ� -1�� �����ش�.
			v[j].pos = positions.at(f[j].posIndex - 1);
			v[j].normal = normals.at(f[j].normalIndex - 1);
			v[j].texC = texcoords.at(f[j].texIndex - 1);
		}

		// ��� ���� �� �����ÿ� ����� TBN�� ����Ѵ�.
		XMFLOAT3 normal, tangent, binormal;
		CalculateTBN(v[0], v[1], v[2], normal, tangent, binormal);

		// vector�� empty�� �� find �Լ��� ����ϸ� incompatible ������ �����.
		// ù ��° ������ ��� �ߺ��Ǵ� ������ ������ �� �����Ƿ� �����Ѵ�.
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

		// obj���Ͽ� ��ϵ� Face �ε������� �ߺ��Ǵ� ���� �ε����� �����Ѵ�.
		// ���� ���� ���۸� ���鶧���� �ߺ��Ǵ� ������ �����ؾ� �Ѵ�.
		for (UINT32 j = 0; j < 3; ++j)
		{
			auto iter = std::find(overlapFaceIndices.begin(), overlapFaceIndices.end(), f[j]);
			// ���� ��ġ�� FaceIndex�� �����Ѵٸ�
			if (iter != overlapFaceIndices.end())
			{
				// �ߺ��Ǵ� ������ �ε����� �����Ѵ�.
				UINT index = (UINT)std::distance(overlapFaceIndices.begin(), iter);
				++overlappedVertexNums[index];
				indices.emplace_back(index);

				// �ߺ��Ǵ� TBN�� ���Ѵ�.
				vertices[index].normal = Vector3::Add(vertices[index].normal, v[j].normal);
				vertices[index].tangentU = Vector3::Add(vertices[index].tangentU, tangent); 
				vertices[index].binormalU = Vector3::Add(vertices[index].binormalU, binormal);
			}
			// �ƴ϶��
			else
			{
				// ���ο� ������ �ε����� �����Ѵ�.
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
		// �ߺ��Ǵ� TBN�� ������ ����� ���Ѵ�.
		auto n = overlappedVertexNums[i];
		vertices[i].normal = Vector3::Divide(vertices[i].normal, n);
		vertices[i].tangentU = Vector3::Divide(vertices[i].tangentU, n);
		vertices[i].binormalU = Vector3::Divide(vertices[i].binormalU, n);
	}
	
	// obj������ h3d���Ϸ� ��ȯ�Ѵ�.
	ObjToH3d(vertices, indices, fileName); 

	return true;
}

void AssetLoader::ObjToH3d(const std::vector<Vertex>& vertices, const std::vector<std::uint16_t>& indices, std::wstring fileName)
{
	static const std::wstring fileFormat = L".h3d";
	fileName.erase(fileName.size() - 4, 4); // fileName���� obj ���� string�� �����.
	fileName += fileFormat; // ���Ͽ� h3d ���������� ���δ�.

	std::ofstream fout(fileName);

	fout << vertices.size() << std::endl; // VertexCount
	for (const auto& vertex : vertices)
	{
		// ���Ͽ� Vertex�� ���� ������ ����.
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
		// �ε����� ����.
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

	// ���� ǥ���� �� ���͸� ����Ѵ�.
	pv1 = p2 - p1;
	pv2 = p3 - p1;

	// tu �� tv �ؽ�ó ���� ���͸� ����Ѵ�.
	tu1 = t2 - t1;
	tv1 = t3 - t1;

	XMFLOAT3 tu, tv;
	XMStoreFloat3(&tu, tu1);
	XMStoreFloat3(&tv, tv1);

	// ź��Ʈ / ���� ��� �������� �и� ����Ѵ�.
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


	// ���� ���� ����ϰ� ����� ���Ͽ� ������ �� ������ ��´�.
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


	// RIFF �������� Ȯ���Ѵ�.
	if ((header.chunkId[0] != 'R') || (header.chunkId[1] != 'I') ||
		(header.chunkId[2] != 'F') || (header.chunkId[3] != 'F'))
		return nullptr;

	// Wave �������� Ȯ���Ѵ�.
	if ((header.format[0] != 'W') || (header.format[1] != 'A') ||
		(header.format[2] != 'V') || (header.format[3] != 'E'))
		return nullptr;

	// fmt �������� Ȯ���Ѵ�.
	if ((header.subChunkId[0] != 'f') || (header.subChunkId[1] != 'm') ||
		(header.subChunkId[2] != 't') || (header.subChunkId[3] != ' '))
		return nullptr;

	/*
	// ����� ������ WAVE_FORMAT_PCM���� Ȯ���Ѵ�.
	if (header.audioFormat != WAVE_FORMAT_PCM)
		return nullptr;

	// ���׷��� �������� ��ϵǾ� �ִ��� Ȯ���Ѵ�.
	if (header.numChannels != 2)
		return nullptr;

	// 44.1kHz ������ ��ϵǾ����� Ȯ���Ѵ�.
	if (header.sampleRate != 44100)
		return nullptr;

	// 16��Ʈ�� ��ϵǾ� �ִ��� Ȯ���Ѵ�.
	if (header.bitsPerSample != 16)
		return nullptr;
	*/

	// data���� Ȯ���Ѵ�.
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