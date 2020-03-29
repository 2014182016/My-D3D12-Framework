//***************************************************************************************
// ShadowMap.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include <Framework/ShadowMap.h>

/*
기본적인 쉐도우 맵
directional light 및 spot light에서 사용된다.
*/
class SimpleShadowMap : public ShadowMap
{
public:
	SimpleShadowMap(const UINT32 width, const UINT32 height);
		
	SimpleShadowMap(const SimpleShadowMap& rhs)=delete;
	SimpleShadowMap& operator=(const SimpleShadowMap& rhs)=delete;
	~SimpleShadowMap()=default;

public:
	// 서술자 핸들을 저장하고, BuildDescriptors를 호출한다.
	virtual void BuildDescriptors(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv) override;
	// 서술자 뷰를 생성한다.
	virtual void BuildDescriptors(ID3D12Device* device) override;
	// 쉐도우 맵 리소스를 생성한다.
	virtual void BuildResource(ID3D12Device* device) override;
	// 새로운 해상도로 세팅하고, 서술자 뷰와 리소스를 다시 생성한다.
	virtual void OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight) override;
	// 쉐도우 맵에 오브젝트들을 그린다.
	virtual void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) override;

private:
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	UINT32 width = 0;
	UINT32 height = 0;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_R24G8_TYPELESS;

	// 서술자 핸들
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv;

	// 쉐도우 맵 리소스
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowMap = nullptr;
};

 