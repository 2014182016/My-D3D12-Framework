//***************************************************************************************
// CubeRenderTarget.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "pch.h"
#include "CubeRenderTarget.h"
#include "D3DUtil.h"
#include "D3DFramework.h"

CubeRenderTarget::CubeRenderTarget(ID3D12Device* device,
	UINT width, UINT height,
	DXGI_FORMAT format)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;
	mRenderTargetFormat = format;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };

	BuildResource();
}

void CubeRenderTarget::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6])
{
	mhCpuSrv = hCpuSrv;
	mhGpuSrv = hGpuSrv;
	mhCpuDsv = hCpuDsv;

	for (int i = 0; i < 6; ++i)
		mhCpuRtv[i] = hCpuRtv[i];

	BuildDescriptors();
}

void CubeRenderTarget::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();
		BuildDescriptors();
	}
}

void CubeRenderTarget::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mRenderTargetFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(mCubeMap.Get(), &srvDesc, mhCpuSrv);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Format = mRenderTargetFormat;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.PlaneSlice = 0;
	rtvDesc.Texture2DArray.ArraySize = 1;
	for (int i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		md3dDevice->CreateRenderTargetView(mCubeMap.Get(), &rtvDesc, mhCpuRtv[i]);
	}
}

void CubeRenderTarget::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 6;
	texDesc.MipLevels = 1;
	texDesc.Format = mRenderTargetFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mCubeMap)));
}

void CubeRenderTarget::RenderSceneToCubeMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* cubePSO,
	const std::list<std::shared_ptr<class GameObject>>& objects, UINT& objectStartIndex)
{
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	// 깊이 버퍼를 쓰기 위해 리소스 배리어를 전환한다.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	for (int i = 0; i < 6; ++i)
	{
		// 후면 버퍼와 깊이 버퍼를 지운다.
		cmdList->ClearRenderTargetView(mhCpuRtv[i], DirectX::Colors::LightSteelBlue, 0, nullptr);
		cmdList->ClearDepthStencilView(mhCpuDsv,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		cmdList->OMSetRenderTargets(1, &mhCpuRtv[i], true, &mhCpuDsv);

		cmdList->SetPipelineState(cubePSO);
		D3DFramework::GetInstance()->RenderGameObjects(cmdList, objects, objectStartIndex);
	}

	// 텍스처를 다시 읽을 수 있도록 리소스를 GENERIC_READ로 바꾸어 준다.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCubeMap.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}