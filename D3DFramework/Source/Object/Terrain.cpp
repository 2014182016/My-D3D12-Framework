#include "../PrecompiledHeader/pch.h"
#include "Terrain.h"
#include "../Component/Mesh.h"
#include "../Framework/D3DInfo.h"

Terrain::Terrain(std::string&& name) : Object(std::move(name)) 
{
	assert(HEIGHT_MAP_SIZE % 16 == 0);
}

Terrain::~Terrain() { }

void Terrain::Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum) const
{
	terrainMesh->Render(cmdList);
}

void Terrain::SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = startAddress;
	cmdList->SetGraphicsRootConstantBufferView((UINT)RpCommon::Object, cbAddress);
}

void Terrain::BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, 
	const float width, const float depth, const INT32 xLen, const INT32 zLen)
{
	terrainMesh = std::make_unique<Mesh>(GetName() + std::to_string(GetUID()));

	this->xLen = xLen;
	this->zLen = zLen;

	std::vector<TerrainVertex> vertices((xLen + 1) * (zLen + 1));
	std::vector<std::uint16_t> indices;
	indices.reserve(xLen * zLen * 12);

	// 가로, 세로 길이에 대하여 각 정점을 계산한다.
	for (int i = 0; i < xLen + 1; ++i)
	{
		for (int j = 0; j < zLen + 1; ++j)
		{
			float x = static_cast<float>(i) / static_cast<float>(xLen) - 0.5f;
			float z = static_cast<float>(j) / static_cast<float>(zLen) - 0.5f;
			vertices[i + j * (xLen + 1)].pos = XMFLOAT3(x * width, 0.0f, z * depth);
			vertices[i + j * (xLen + 1)].tex = XMFLOAT2(x + 0.5f, z + 0.5f);
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

	// 지형에 사용되는 메쉬를 생성한다.
	terrainMesh->BuildVertices(device, cmdList, (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(TerrainVertex));
	terrainMesh->BuildIndices(device, cmdList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));
	terrainMesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST);
}

void Terrain::BuildDescriptors(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv)
{
	BuildResources(device);

	CD3DX12_CPU_DESCRIPTOR_HANDLE lodLookupMapCpuSrv = hCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE normalMapCpuSrv = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE lodLookupMapCpuUav = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE normalMapCpuUav = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);

	hResourcesGpuSrv = hGpuSrv;
	hResourcesGpuUav = hGpuSrv.Offset(2, DescriptorSize::cbvSrvUavDescriptorSize);

	// 각 LOD맵과 노멀 맵의 SRV 뷰를 생성한다.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(lodLookupMap.Get(), &srvDesc, lodLookupMapCpuSrv);
	device->CreateShaderResourceView(normalMap.Get(), &srvDesc, normalMapCpuSrv);

	// 각 LOD맵과 노멀 맵의 UAV 뷰를 생성한다.
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;
	device->CreateUnorderedAccessView(lodLookupMap.Get(), nullptr, &uavDesc, lodLookupMapCpuUav);
	device->CreateUnorderedAccessView(normalMap.Get(), nullptr, &uavDesc, normalMapCpuUav);
}

void Terrain::BuildResources(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = xLen;
	texDesc.Height = zLen;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// LOD맵 리소스를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&lodLookupMap)));

	texDesc.Width = HEIGHT_MAP_SIZE;
	texDesc.Height = HEIGHT_MAP_SIZE;

	// 노멀 맵 리소스를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&normalMap)));
}

void Terrain::SetUavDescriptors(ID3D12GraphicsCommandList* cmdList)
{
	// 한번에 LODMap와 Normal을 같이 묶는다.
	cmdList->SetComputeRootDescriptorTable((int)RpTerrainCompute::Uav, hResourcesGpuUav);
}

void Terrain::SetSrvDescriptors(ID3D12GraphicsCommandList* cmdList)
{
	// 한번에 LODMap와 Normal을 같이 묶는다.
	cmdList->SetGraphicsRootDescriptorTable((int)RpTerrainGraphics::Srv, hResourcesGpuSrv);
}

void Terrain::LODCompute(ID3D12GraphicsCommandList* cmdList)
{
	// 표준 편차를 계산하여 LOD를 정한다.
	cmdList->Dispatch(static_cast<int>(std::ceilf(HEIGHT_MAP_SIZE / 16.0f)),
		static_cast<int>(std::ceilf(HEIGHT_MAP_SIZE / 16.0f)), 1);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(lodLookupMap.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Terrain::NormalCompute(ID3D12GraphicsCommandList* cmdList)
{
	// 노멀을 계산하여 노멀 맵을 만든다.
	cmdList->Dispatch(static_cast<int>(std::ceilf(HEIGHT_MAP_SIZE / 16.0f)),
		static_cast<int>(std::ceilf(HEIGHT_MAP_SIZE / 16.0f)), 1);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

XMFLOAT2 Terrain::GetPixelDimesion() const
{
	return XMFLOAT2(HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE); 
}

XMFLOAT2 Terrain::GetGeometryDimesion() const
{
	return XMFLOAT2(static_cast<float>(xLen), static_cast<float>(zLen)); 
}

void Terrain::SetMaterial(Material* material)
{
	this->material = material;
}

Material* Terrain::GetMaterial() const
{
	return material;
}