#include "../PrecompiledHeader/pch.h"
#include "Ssr.h"
#include "D3DUtil.h"
#include "Enumeration.h"

Ssr::Ssr(ID3D12Device* device, const UINT32 width, const UINT32 height)
{
	OnResize(device, width, height);
}

void Ssr::OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight)
{
	if (renderTargetWidth != newWidth || renderTargetHeight != newHeight)
	{
		renderTargetWidth = newWidth;
		renderTargetHeight = newHeight;

		viewportRect.TopLeftX = 0.0f;
		viewportRect.TopLeftY = 0.0f;
		viewportRect.Width = (float)renderTargetWidth;
		viewportRect.Height = (float)renderTargetHeight;
		viewportRect.MinDepth = 0.0f;
		viewportRect.MaxDepth = 1.0f;

		scissorRect = { 0, 0, (int)renderTargetWidth, (int)renderTargetHeight };

		// 리소스를 생성한다.
		BuildResources(device);
	}
}

void Ssr::BuildDescriptors(ID3D12Device* device,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
	// 파이프라인에 바인딩하기 위한 서술자 핸들을 저장한다.
	this->hPositionMapGpuSrv = hPositionMapGpuSrv;
	this->hNormalMapGpuSrv = hNormalMapGpuSrv;
	hSsrMapGpuSrv = hGpuSrv;
	hSsrMapCpuRtv = hCpuRtv;

	// ssr 맵 SRV 뷰를 생성한다.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = ssrMapFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(ssrMap.Get(), &srvDesc, hCpuSrv);

	// ssr 맵 RTV 뷰를 생성한다.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = ssrMapFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	device->CreateRenderTargetView(ssrMap.Get(), &rtvDesc, hCpuRtv);
}

void Ssr::BuildResources(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = renderTargetWidth;
	texDesc.Height = renderTargetHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = ssrMapFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	CD3DX12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(ssrMapFormat, (float*)&clearColor);

	// ssr 맵 리소스를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&ssrMap)));
}

void Ssr::ComputeSsr(ID3D12GraphicsCommandList* cmdList, CD3DX12_GPU_DESCRIPTOR_HANDLE currColorMapGpuSrv,
	D3D12_GPU_VIRTUAL_ADDRESS ssrCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress)
{
	cmdList->RSSetViewports(1, &viewportRect);
	cmdList->RSSetScissorRects(1, &scissorRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ssrMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ssr 맵을 지우고, 출력 단계에 바인딩한다.
	cmdList->ClearRenderTargetView(hSsrMapCpuRtv, (float*)&clearColor, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &hSsrMapCpuRtv, true, nullptr);

	// srr에서 필요한 리소스들을 파이프라인에 바인딩한다.
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsr::PositionMap, hPositionMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsr::NormalMap, hNormalMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsr::ColorMap, currColorMapGpuSrv);
	cmdList->SetGraphicsRootConstantBufferView((int)RpSsr::SsrCB, ssrCBAddress);
	cmdList->SetGraphicsRootConstantBufferView((int)RpSsr::Pass, passCBAddress);

	// ssr맵을 그린다.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ssrMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

ID3D12Resource* Ssr::Output()
{
	return ssrMap.Get();
}