//***************************************************************************************
// ShadowMap.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "pch.h"
#include "Enums.h"

#define SHADOW_MAP_SIZE 2048

class ShadowMap
{
public:
	ShadowMap(ID3D12Device* device, UINT width, UINT height);
		
	ShadowMap(const ShadowMap& rhs)=delete;
	ShadowMap& operator=(const ShadowMap& rhs)=delete;
	~ShadowMap()=default;

public:
	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);
	void OnResize(UINT newWidth, UINT newHeight);
	void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* shadowPSO,
		const std::list<std::shared_ptr<class GameObject>>& objects, UINT& objectStartIndex);

public:
	inline UINT GetWidth() const { return mWidth; }
	inline UINT GetHeight() const { return mHeight; }
	inline D3D12_VIEWPORT GetViewport() const { return mViewport; }
	inline D3D12_RECT GetScissorRect() const { return mScissorRect; }
	inline ID3D12Resource* GetResource() { return mShadowMap.Get(); }
	inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResourceView() const { return mhGpuSrv; }
	inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const { return mhCpuDsv; }

private:
	void BuildDescriptors();
	void BuildResource();

private:

	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_R24G8_TYPELESS;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMap = nullptr;
};

 