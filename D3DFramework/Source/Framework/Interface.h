#pragma once

#include "pch.h"

class Renderable
{
public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) const = 0;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const = 0;
};

class ShadowMap
{
public:
	virtual void BuildDescriptors(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv) = 0;
	virtual void BuildDescriptors(ID3D12Device* device) = 0;
	virtual void BuildResource(ID3D12Device* device) = 0;
	virtual void OnResize(ID3D12Device* device, UINT width, UINT height) = 0;
	virtual void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* camFrustum = nullptr) = 0;
};