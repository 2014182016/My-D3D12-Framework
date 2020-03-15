#pragma once

#include "pch.h"

class Ssr
{
public:
	Ssr(ID3D12Device* device, UINT width, UINT height);
	Ssr(const Ssr& rhs) = delete;
	Ssr& operator=(const Ssr& rhs) = delete;
	~Ssr() = default;

public:
	void BuildDescriptors(ID3D12Device* device,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

	void OnResize(ID3D12Device* device, UINT newWidth, UINT newHeight);

	void ComputeSsr(ID3D12GraphicsCommandList* cmdList, CD3DX12_GPU_DESCRIPTOR_HANDLE currColorMapGpuSrv,
		D3D12_GPU_VIRTUAL_ADDRESS ssrCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress);

public:
	inline ID3D12Resource* GetResource() { return mSsrMap.Get(); }

public:
	static const DXGI_FORMAT ssrMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
	void BuildResources(ID3D12Device* device);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mSsrMap;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mhPositionMapGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalMapGpuSrv;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mhSsrMapGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhSsrMapCpuRtv;

	UINT mRenderTargetWidth;
	UINT mRenderTargetHeight;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
};