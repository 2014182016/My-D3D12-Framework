#pragma once
//***************************************************************************************
// Ssao.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include <Framework/D3DUtil.h>
#include <vector>

#define SAMPLE_COUNT 14
#define RANDOM_VECTORMAP_SIZE 256

/*
객체들의 차폐를 계산하는 ssao 맵을 캡슐화한 클래스
*/
class Ssao
{
public:
	Ssao(ID3D12Device* device,ID3D12GraphicsCommandList* cmdList, const UINT32 width, const UINT32 height);
	Ssao(const Ssao& rhs) = delete;
	Ssao& operator=(const Ssao& rhs) = delete;
	~Ssao() = default;

public:
	// 블러하기 위한 가우스 가중치를 계산한다.
	std::vector<float> CalcGaussWeights(const float sigma) const;

	// 서술자 핸들을 저장하고 서술자 뷰를 생성한다.
	void BuildDescriptors(ID3D12Device* device,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hDiffuseMapGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

	// 서술자 뷰를 재생성한다.
	void RebuildDescriptors(ID3D12Device* device);
	// 새로운 해상도로 바꾸고 리소스를 다시 생성한다.
	void OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight);

	void ComputeSsao(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* ssaoPso,
		D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress);
	void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* blurPso, const INT32 blurCount);

	void GetOffsetVectors(XMFLOAT4* offsets);

	UINT32 GetWidth() const;
	UINT32 GetHeight() const;

private:
	// ssao맵을 블러시킨다.
	void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur);

	void BuildResources(ID3D12Device* device);
	void BuildRandomVectorTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildOffsetVectors();

public:
	static inline const DXGI_FORMAT ambientMapFormat = DXGI_FORMAT_R16_UNORM;
	// 블러 반지름은 이 최대 반지름을 넘을 수 없다.
	static inline const INT32 maxBlurRadius = 5;


private:
	// ssao맵에 필요한 리소스들이다.
	Microsoft::WRL::ComPtr<ID3D12Resource> randomVectorMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> randomVectorMapUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> ambientMap0;
	Microsoft::WRL::ComPtr<ID3D12Resource> ambientMap1;

	// ssao에서 필요한 노멀맵과 디퓨즈 맵의 핸들이다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv;

	// ssao에서 필요한 랜덤 벡터를 추출할 수 있는 서술자 핸들이다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE hRandomVectorMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hRandomVectorMapGpuSrv;

	// 블러하는 동안 핑퐁(ping-ponging)을 위해 두 개의 리소스가 필요하다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE hAmbientMap0CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hAmbientMap0GpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hAmbientMap0CpuRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hAmbientMap1CpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hAmbientMap1GpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hAmbientMap1CpuRtv;

	UINT32 renderTargetWidth;
	UINT32 renderTargetHeight;

	D3D12_VIEWPORT viewportRect;
	D3D12_RECT scissorRect;

	// ssao에서 해당 정점에서 다른 정점으로 이동시키는 오프셋이다.
	XMFLOAT4 offsets[SAMPLE_COUNT];

	const XMFLOAT4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };
};