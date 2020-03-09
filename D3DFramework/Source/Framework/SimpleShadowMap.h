//***************************************************************************************
// ShadowMap.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "pch.h"
#include "Interface.h"

class SimpleShadowMap : public ShadowMap
{
public:
	SimpleShadowMap(UINT width, UINT height);
		
	SimpleShadowMap(const SimpleShadowMap& rhs)=delete;
	SimpleShadowMap& operator=(const SimpleShadowMap& rhs)=delete;
	~SimpleShadowMap()=default;

public:
	virtual void BuildDescriptors(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv) override;
	virtual void BuildDescriptors(ID3D12Device* device) override;
	virtual void BuildResource(ID3D12Device* device) override;
	virtual void OnResize(ID3D12Device* device, UINT width, UINT height) override;
	virtual void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) override;

private:
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

 