#include "pch.h"
#include "ObjLoader.h"
#include "Structure.h"
#include "Mesh.h"

using namespace DirectX;

std::unique_ptr<Mesh> ObjLoader::LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshInfo& modelInfo)
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
		return nullptr;
	}

	UINT vertexCount;
	fin >> vertexCount;
	vertices.reserve(vertexCount);
	for (UINT i = 0; i < vertexCount; ++i)
	{
		// h3d���Ͽ��� Vertex�� ���� ������ �о�´�.
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
		// �ε��� ������ �о�´�.
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

	return mesh;
}

void ObjLoader::ConvertObj(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshInfo& modelInfo)
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
				fin >> f[2].mPosIndex >> input >> f[2].mTexIndex >> input >> f[2].mNormalIndex;
				fin >> f[1].mPosIndex >> input >> f[1].mTexIndex >> input >> f[1].mNormalIndex;
				fin >> f[0].mPosIndex >> input >> f[0].mTexIndex >> input >> f[0].mNormalIndex;

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

		// �� ��(Face)�� ���Ǵ� �� �������� �ε����� ���� ������ �ҷ��´�.
		for (UINT j = 0; j < 3; ++j)
		{
			// obj������ �ε����� 1���� �����ϹǷ� -1�� �����ش�.
			v[j].mPos = positions.at(f[j].mPosIndex - 1);
			v[j].mNormal = normals.at(f[j].mNormalIndex - 1);
			v[j].mTexC = texcoords.at(f[j].mTexIndex - 1);
		}

		// ��� ���� �� �����ÿ� ����� TBN�� ����Ѵ�.
		XMFLOAT3 normal, tangent, binormal;
		CalculateTBN(v[0], v[1], v[2], normal, tangent, binormal);

		// vector�� empty�� �� find �Լ��� ����ϸ� incompatible ������ �����.
		// ù ��° ������ ��� �ߺ��Ǵ� ������ ������ �� �����Ƿ� �����Ѵ�.
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

		// obj���Ͽ� ��ϵ� Face �ε������� �ߺ��Ǵ� ���� �ε����� �����Ѵ�.
		// ���� ���� ���۸� ���鶧���� �ߺ��Ǵ� ������ �����ؾ� �Ѵ�.
		for (UINT j = 0; j < 3; ++j)
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
				vertices[index].mNormal = vertices[index].mNormal + v[j].mNormal;
				vertices[index].mTangentU = vertices[index].mTangentU + tangent;
				vertices[index].mBinormalU = vertices[index].mBinormalU + binormal;
			}
			// �ƴ϶��
			else
			{
				// ���ο� ������ �ε����� �����Ѵ�.
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
		// �ߺ��Ǵ� TBN�� ������ ����� ���Ѵ�.
		auto n = overlappedVertexNums[i];
		vertices[i].mNormal = vertices[i].mNormal / n;
		vertices[i].mTangentU = vertices[i].mTangentU / n;
		vertices[i].mBinormalU = vertices[i].mBinormalU / n;
	}

	fileName.erase(fileName.size() - 4, 4); // fileName���� obj ���� string�� �����.
	ObjToH3d(vertices, indices, fileName); // obj������ h3d���Ϸ� ��ȯ�Ѵ�.

	/*
	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(std::move(meshName));
	mesh->BuildVertices(device, commandList, (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(Vertex));
	mesh->BuildIndices(device, commandList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));

	if (collisionType != CollisionType::None)
		mesh->BuildCollisionBound(&vertices[0].mPos, vertices.size(), (size_t)sizeof(Vertex), collisionType);

	return mesh;
	*/
}

void ObjLoader::ObjToH3d(const std::vector<Vertex>& vertices, const std::vector<std::uint16_t> indices, std::wstring fileName)
{
	std::wstring fileFormat = L".h3d";
	fileName += fileFormat;

	std::ofstream fout(fileName);

	fout << vertices.size() << std::endl; // VertexCount
	for (const auto& vertex : vertices)
	{
		// ���Ͽ� Vertex�� ���� ������ ����.
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
		// �ε����� ����.
		fout << i << std::endl;
	}

	fout.close();
}

void ObjLoader::CalculateTBN(const VertexBasic& v1, const VertexBasic& v2, const VertexBasic& v3,
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
