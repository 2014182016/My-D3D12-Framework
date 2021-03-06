//***************************************************************************************
// SimpleShadowMap.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "../PrecompiledHeader/pch.h"
#include "SimpleShadowMap.h"
#include "D3DFramework.h"
#include "../Object/GameObject.h"

SimpleShadowMap::SimpleShadowMap(const UINT32 width, const UINT32 height)
{
	this->width = width;
	this->height = height;

	viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	scissorRect = { 0, 0, (int)width, (int)height };
}

void SimpleShadowMap::BuildDescriptors(ID3D12Device* device, 
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
	this->hCpuSrv = hCpuSrv;
	this->hGpuSrv = hGpuSrv;
	this->hCpuDsv = hCpuDsv;

	BuildDescriptors(device);
}

void SimpleShadowMap::OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight)
{
	if((width != newWidth) || (height != newHeight))
	{
		width = newWidth;
		height = newHeight;

		BuildResource(device);
		BuildDescriptors(device);
	}
}

void SimpleShadowMap::RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum)
{
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	// 깊이 버퍼를 쓰기 위해 리소스 배리어를 전환한다.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	cmdList->ClearDepthStencilView(hCpuDsv,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 장면을 깊이 버퍼에만 렌더링할 것이므로 렌더 대상은 널로 설정한다.
	// 어차피 널 렌더 대상을 지정하면 색상 쓰기가 비활성화된다.
	// 반드시 활성 PSO의 렌더 대상 개수도 0으로 지정해야 함을 주의하기 바란다.
	cmdList->OMSetRenderTargets(0, nullptr, false, &hCpuDsv);

	D3DFramework::GetInstance()->RenderActualObjects(cmdList, frustum);

	// 텍스처를 다시 읽을 수 있도록 리소스를 GENERIC_READ로 바꾸어 준다.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowMap.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}
 
void SimpleShadowMap::BuildDescriptors(ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; 
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.PlaneSlice = 0;
	device->CreateShaderResourceView(shadowMap.Get(), &srvDesc, hCpuSrv);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, hCpuDsv);
}

void SimpleShadowMap::BuildResource(ID3D12Device* device)
{
	shadowMap.Reset();

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = depthStencilFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	// 리소스를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(shadowMap.GetAddressOf())));
}