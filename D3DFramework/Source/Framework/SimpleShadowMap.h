//***************************************************************************************
// ShadowMap.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include <Framework/ShadowMap.h>

/*
�⺻���� ������ ��
directional light �� spot light���� ���ȴ�.
*/
class SimpleShadowMap : public ShadowMap
{
public:
	SimpleShadowMap(const UINT32 width, const UINT32 height);
		
	SimpleShadowMap(const SimpleShadowMap& rhs)=delete;
	SimpleShadowMap& operator=(const SimpleShadowMap& rhs)=delete;
	~SimpleShadowMap()=default;

public:
	// ������ �ڵ��� �����ϰ�, BuildDescriptors�� ȣ���Ѵ�.
	virtual void BuildDescriptors(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv) override;
	// ������ �並 �����Ѵ�.
	virtual void BuildDescriptors(ID3D12Device* device) override;
	// ������ �� ���ҽ��� �����Ѵ�.
	virtual void BuildResource(ID3D12Device* device) override;
	// ���ο� �ػ󵵷� �����ϰ�, ������ ��� ���ҽ��� �ٽ� �����Ѵ�.
	virtual void OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight) override;
	// ������ �ʿ� ������Ʈ���� �׸���.
	virtual void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) override;

private:
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	UINT32 width = 0;
	UINT32 height = 0;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_R24G8_TYPELESS;

	// ������ �ڵ�
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv;

	// ������ �� ���ҽ�
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowMap = nullptr;
};

 