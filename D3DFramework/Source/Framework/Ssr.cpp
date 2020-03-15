#include "pch.h"
#include "Ssr.h"
#include "D3DUtil.h"
#include "Global.h"

Ssr::Ssr(ID3D12Device* device, UINT width, UINT height)
{
	OnResize(device, width, height);
}

void Ssr::OnResize(ID3D12Device* device, UINT newWidth, UINT newHeight)
{
	if (mRenderTargetWidth != newWidth || mRenderTargetHeight != newHeight)
	{
		mRenderTargetWidth = newWidth;
		mRenderTargetHeight = newHeight;

		mViewport.TopLeftX = 0.0f;
		mViewport.TopLeftY = 0.0f;
		mViewport.Width = (float)mRenderTargetWidth;
		mViewport.Height = (float)mRenderTargetHeight;
		mViewport.MinDepth = 0.0f;
		mViewport.MaxDepth = 1.0f;

		mScissorRect = { 0, 0, (int)mRenderTargetWidth, (int)mRenderTargetHeight };

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
	mhPositionMapGpuSrv = hPositionMapGpuSrv;
	mhNormalMapGpuSrv = hNormalMapGpuSrv;
	mhSsrMapGpuSrv = hGpuSrv;
	mhSsrMapCpuRtv = hCpuRtv;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = ssrMapFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(mSsrMap.Get(), &srvDesc, hCpuSrv);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = ssrMapFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	device->CreateRenderTargetView(mSsrMap.Get(), &rtvDesc, hCpuRtv);
}

void Ssr::BuildResources(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mRenderTargetWidth;
	texDesc.Height = mRenderTargetHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = ssrMapFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	CD3DX12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(ssrMapFormat, clearColor);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mSsrMap)));
}

void Ssr::ComputeSsr(ID3D12GraphicsCommandList* cmdList, CD3DX12_GPU_DESCRIPTOR_HANDLE currColorMapGpuSrv,
	D3D12_GPU_VIRTUAL_ADDRESS ssrCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress)
{
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSsrMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	cmdList->ClearRenderTargetView(mhSsrMapCpuRtv, clearValue, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &mhSsrMapCpuRtv, true, nullptr);

	cmdList->SetGraphicsRootDescriptorTable((int)RpSsr::PositionMap, mhPositionMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsr::NormalMap, mhNormalMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsr::ColorMap, currColorMapGpuSrv);
	cmdList->SetGraphicsRootConstantBufferView((int)RpSsr::SsrCB, ssrCBAddress);
	cmdList->SetGraphicsRootConstantBufferView((int)RpSsr::Pass, passCBAddress);

	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSsrMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}