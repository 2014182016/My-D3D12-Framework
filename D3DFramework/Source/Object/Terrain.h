#pragma once

#include <Object/Object.h>
#include <Framework/Renderable.h>

class Mesh;
class Material;

#define HEIGHT_MAP_SIZE 512

/*
지형에 관한 데이터를 캡슐화한 클래스
*/
class Terrain : public Object, public Renderable
{
public:
	Terrain(std::string&& name);
	virtual ~Terrain();

public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;

public:
	// 지형에 사용할 평면을 계산하고, 메쉬를 생성한다.
	void BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
		const float width, const float depth, const INT32 xLen, const INT32 zLen);
	// 서술자 뷰를 생성한다.
	void BuildDescriptors(ID3D12Device* device, 
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, 
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv);
	// 사용할 UAV 리소스를 파이프라인에 바인딩한다.
	void SetUavDescriptors(ID3D12GraphicsCommandList* cmdList);
	// 사용할 SRV 리소스를 파이프라인에 바인딩한다.
	void SetSrvDescriptors(ID3D12GraphicsCommandList* cmdList);

	// 하이트 맵을 이용하여 표준 편차를 계산하고 LOD를 리소스에 담는다.
	void LODCompute(ID3D12GraphicsCommandList* cmdList);
	// 하이트 맵을 이용하여 노멀을 계산하고 노멀 맵 리소스에 담는다.
	void NormalCompute(ID3D12GraphicsCommandList* cmdList);

	XMFLOAT2 GetPixelDimesion() const;
	XMFLOAT2 GetGeometryDimesion() const;

	void SetMaterial(Material* material);
	Material* GetMaterial() const;

private:
	// 지형에 필요한 리소스를 생성한다.
	void BuildResources(ID3D12Device* device);


protected:
	// 평면 메쉬가 나누어지는 정도
	UINT32 xLen = 0;
	UINT32 zLen = 0;

private:
	// 지형에 사용되는 평면 메쉬
	std::unique_ptr<Mesh> terrainMesh;
	Material* material = nullptr;

	// 미리 계산된 표준 편차를 이용한 LOD 맵. 지형은 이 LOD맵을
	// 사용하여 최적화된 테셀레이션이 가능하다.
	Microsoft::WRL::ComPtr<ID3D12Resource> lodLookupMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> normalMap;

	CD3DX12_GPU_DESCRIPTOR_HANDLE hResourcesGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hResourcesGpuUav;
};