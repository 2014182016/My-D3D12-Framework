#pragma once

#include <Object/Object.h>
#include <Framework/Renderable.h>

class Mesh;
class Material;

#define HEIGHT_MAP_SIZE 512

/*
������ ���� �����͸� ĸ��ȭ�� Ŭ����
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
	// ������ ����� ����� ����ϰ�, �޽��� �����Ѵ�.
	void BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
		const float width, const float depth, const INT32 xLen, const INT32 zLen);
	// ������ �並 �����Ѵ�.
	void BuildDescriptors(ID3D12Device* device, 
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, 
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv);
	// ����� UAV ���ҽ��� ���������ο� ���ε��Ѵ�.
	void SetUavDescriptors(ID3D12GraphicsCommandList* cmdList);
	// ����� SRV ���ҽ��� ���������ο� ���ε��Ѵ�.
	void SetSrvDescriptors(ID3D12GraphicsCommandList* cmdList);

	// ����Ʈ ���� �̿��Ͽ� ǥ�� ������ ����ϰ� LOD�� ���ҽ��� ��´�.
	void LODCompute(ID3D12GraphicsCommandList* cmdList);
	// ����Ʈ ���� �̿��Ͽ� ����� ����ϰ� ��� �� ���ҽ��� ��´�.
	void NormalCompute(ID3D12GraphicsCommandList* cmdList);

	XMFLOAT2 GetPixelDimesion() const;
	XMFLOAT2 GetGeometryDimesion() const;

	void SetMaterial(Material* material);
	Material* GetMaterial() const;

private:
	// ������ �ʿ��� ���ҽ��� �����Ѵ�.
	void BuildResources(ID3D12Device* device);


protected:
	// ��� �޽��� ���������� ����
	UINT32 xLen = 0;
	UINT32 zLen = 0;

private:
	// ������ ���Ǵ� ��� �޽�
	std::unique_ptr<Mesh> terrainMesh;
	Material* material = nullptr;

	// �̸� ���� ǥ�� ������ �̿��� LOD ��. ������ �� LOD����
	// ����Ͽ� ����ȭ�� �׼����̼��� �����ϴ�.
	Microsoft::WRL::ComPtr<ID3D12Resource> lodLookupMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> normalMap;

	CD3DX12_GPU_DESCRIPTOR_HANDLE hResourcesGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hResourcesGpuUav;
};