#pragma once

#include "pch.h"
#include "Object.h"
#include "Interface.h"

#define HEIGHT_MAP_SIZE 512

class Terrain : public Object, public Renderable
{
public:
	Terrain(std::string&& name);
	virtual ~Terrain();

public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;

public:
	void BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, float width, float depth, int xLen, int zLen);
	void BuildDescriptors(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv);
	void SetUavDescriptors(ID3D12GraphicsCommandList* cmdList);
	void SetSrvDescriptors(ID3D12GraphicsCommandList* cmdList);
	void LODCompute(ID3D12GraphicsCommandList* cmdList);
	void NormalCompute(ID3D12GraphicsCommandList* cmdList);

private:
	void BuildResources(ID3D12Device* device);

public:
	inline void SetMaterial(class Material* material) { mMaterial = material; }
	inline class Material* GetMaterial() { return mMaterial; }

	inline DirectX::XMFLOAT2 GetPixelDimesion() const { return DirectX::XMFLOAT2(HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE); }
	inline DirectX::XMFLOAT2 GetGeometryDimesion() const { return DirectX::XMFLOAT2(static_cast<float>(mXLen), static_cast<float>(mZLen)); }

protected:
	std::unique_ptr<class Mesh> mTerrainMesh;
	class Material* mMaterial = nullptr;
	
	UINT mXLen = 0;
	UINT mZLen = 0;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mLODLookupMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNormalMap;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mhResourcesGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhResourcesGpuUav;
};