#pragma once
//***************************************************************************************
// Ssao.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "pch.h"

#define SAMPLE_COUNT 14
#define RANDOM_VECTORMAP_SIZE 256

class Ssao
{
public:
	Ssao(ID3D12Device* device,ID3D12GraphicsCommandList* cmdList, UINT width, UINT height);
	Ssao(const Ssao& rhs) = delete;
	Ssao& operator=(const Ssao& rhs) = delete;
	~Ssao() = default;

public:
	std::vector<float> CalcGaussWeights(float sigma);

	void BuildDescriptors(ID3D12Device* device,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		UINT cbvSrvUavDescriptorSize,
		UINT rtvDescriptorSize);

	void RebuildDescriptors(ID3D12Device* device);
	void OnResize(ID3D12Device* device, UINT newWidth, UINT newHeight);
	void ComputeSsao(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* ssaoPso,
		D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress);
	void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* blurPso, int blurCount);

	void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]) { std::copy(&mOffsets[0], &mOffsets[14], &offsets[0]); }
	UINT GetMapWidth() const { return mRenderTargetWidth; }
	UINT GetMapHeight() const { return mRenderTargetHeight; }

public:
	static const DXGI_FORMAT AmbientMapFormat = DXGI_FORMAT_R16_UNORM;
	static const int MaxBlurRadius = 5;

private:
	void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur);

	void BuildResources(ID3D12Device* device);
	void BuildRandomVectorTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	void BuildOffsetVectors();


private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mRandomVectorMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mRandomVectorMapUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mAmbientMap0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mAmbientMap1;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalMapGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhDepthMapGpuSrv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhRandomVectorMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhRandomVectorMapGpuSrv;

	// Need two for ping-ponging during blur.
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap0CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap0GpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap0CpuRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap1CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap1GpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap1CpuRtv;

	UINT mRenderTargetWidth;
	UINT mRenderTargetHeight;

	DirectX::XMFLOAT4 mOffsets[SAMPLE_COUNT];

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
};