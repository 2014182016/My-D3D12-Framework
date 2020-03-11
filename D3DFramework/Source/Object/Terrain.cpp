#include "pch.h"
#include "Terrain.h"
#include "Mesh.h"
#include "Structure.h"
#include "Global.h"

using namespace DirectX;

Terrain::Terrain(std::string&& name) : Object(std::move(name)) { }

Terrain::~Terrain() { }

void Terrain::Render(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum) const
{
	mTerrainMesh->Render(cmdList);
}

void Terrain::SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = startAddress;
	cmdList->SetGraphicsRootConstantBufferView((UINT)RpCommon::Object, cbAddress);
}

void Terrain::BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float width, float depth, int xLen, int zLen)
{
	mTerrainMesh = std::make_unique<Mesh>(GetName() + std::to_string(GetUID()));

	std::vector<TerrainVertex> vertices((xLen + 1) * (zLen + 1));
	std::vector<std::uint16_t> indices;
	indices.reserve(xLen * zLen * 12);

	for (int i = 0; i < xLen + 1; ++i)
	{
		for (int j = 0; j < zLen + 1; ++j)
		{
			float x = static_cast<float>(i) / static_cast<float>(xLen) - 0.5f;
			float z = static_cast<float>(j) / static_cast<float>(zLen) - 0.5f;
			vertices[i + j * (xLen + 1)].mPos = XMFLOAT3(x * width, 0.0f, z * depth);
			vertices[i + j * (xLen + 1)].mTex = XMFLOAT2(x + 0.5f, z + 0.5f);
		}
	}

	for (int i = 0; i < xLen; ++i)
	{
		for (int j = 0; j < zLen; ++j)
		{
			// 지형 타일 사각형 당 12개의 제어점을 정의한다.

			// 0에서 3까지는 실제 사각형 정점들이다.
			indices.emplace_back((j + 0) + (i + 0) * (xLen + 1));
			indices.emplace_back((j + 1) + (i + 0) * (xLen + 1));
			indices.emplace_back((j + 0) + (i + 1) * (xLen + 1));
			indices.emplace_back((j + 1) + (i + 1) * (xLen + 1));

			// 4와 5는 +X방향 이웃 타일이다.
			indices.emplace_back(std::clamp(j + 0, 0, zLen) + std::clamp(i + 2, 0, xLen) * (xLen + 1));
			indices.emplace_back(std::clamp(j + 1, 0, zLen) + std::clamp(i + 2, 0, xLen) * (xLen + 1));

			// 6과 7은 +Z방향 이웃 타일이다.
			indices.emplace_back(std::clamp(j + 2, 0, zLen) + std::clamp(i + 0, 0, xLen) * (xLen + 1));
			indices.emplace_back(std::clamp(j + 2, 0, zLen) + std::clamp(i + 1, 0, xLen) * (xLen + 1));

			// 8과 9은 -X방향 이웃 타일이다.
			indices.emplace_back(std::clamp(j + 0, 0, zLen) + std::clamp(i - 1, 0, xLen) * (xLen + 1));
			indices.emplace_back(std::clamp(j + 1, 0, zLen) + std::clamp(i - 1, 0, xLen) * (xLen + 1));

			// 10과 11은 -Z방향 이웃 타일이다.
			indices.emplace_back(std::clamp(j - 1, 0, zLen) + std::clamp(i + 0, 0, xLen) * (xLen + 1));
			indices.emplace_back(std::clamp(j - 1, 0, zLen) + std::clamp(i + 1, 0, xLen) * (xLen + 1));
		}
	}

	mTerrainMesh->BuildVertices(device, commandList, (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(TerrainVertex));
	mTerrainMesh->BuildIndices(device, commandList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));
	mTerrainMesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST);
}