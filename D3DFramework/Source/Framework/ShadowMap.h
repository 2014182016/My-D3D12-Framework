#pragma once

#include "d3dx12.h"
#include <basetsd.h>
#include <DirectXCollision.h>

/*
쉐도우 맵 관련 클래스는 이 인터페이스를 상속받는다.
*/
class ShadowMap
{
public:
	virtual void BuildDescriptors(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv) = 0;
	virtual void BuildDescriptors(ID3D12Device* device) = 0;
	virtual void BuildResource(ID3D12Device* device) = 0;
	virtual void OnResize(ID3D12Device* device, UINT32 width, UINT32 height) = 0;
	virtual void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* camFrustum = nullptr) = 0;
};