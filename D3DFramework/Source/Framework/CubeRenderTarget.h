//***************************************************************************************
// CubeRenderTarget.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "pch.h"

class CubeRenderTarget
{
public:
public:
	CubeRenderTarget(ID3D12Device* device,
		UINT width, UINT height,
		DXGI_FORMAT format);

	CubeRenderTarget(const CubeRenderTarget& rhs) = delete;
	CubeRenderTarget& operator=(const CubeRenderTarget& rhs) = delete;
	~CubeRenderTarget() = default;

public:
	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]);

	void OnResize(UINT newWidth, UINT newHeight);
	void RenderSceneToCubeMap(ID3D12GraphicsCommandList* cmdList, const std::list<std::shared_ptr<class GameObject>>& objects);

public:
	inline ID3D12Resource* Resource() { return mCubeMap.Get(); }
	inline CD3DX12_GPU_DESCRIPTOR_HANDLE Srv() { return mhGpuSrv; }
	inline CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(int faceIndex) { return mhCpuRtv[faceIndex]; }

	inline D3D12_VIEWPORT Viewport() const { return mViewport; }
	inline D3D12_RECT ScissorRect() const { return mScissorRect; }

private:
	void BuildDescriptors();
	void BuildResource();

private:

	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv[6];

	Microsoft::WRL::ComPtr<ID3D12Resource> mCubeMap = nullptr;
};

