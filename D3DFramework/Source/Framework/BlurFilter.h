#pragma once

//***************************************************************************************
// BlurFilter.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Performs a blur operation on the topmost mip level of an input texture.
//***************************************************************************************

#include <Framework/D3DUtil.h>
#include <vector>

#define NUM_THREADS 256.0f

/*
텍스처를 흐리게 만들도록 하는 캡슐화된 클래스
*/
class BlurFilter
{
public:
	BlurFilter(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight, const DXGI_FORMAT newFormat);

	BlurFilter(const BlurFilter& rhs) = delete;
	BlurFilter& operator=(const BlurFilter& rhs) = delete;
	~BlurFilter() = default;

public:
	// 블러를 수행한 후 완성된 리소스를 반환한다.
	ID3D12Resource* Output();

	// 새로운 해상도로 리소스를 재생성한다.
	void OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight);

	// 필요한 리소스 뷰를 생성하고 서술자 핸들을 저장한다.
	void BuildDescriptors(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor);

	// 블러를 수행한다.
	void Execute(ID3D12GraphicsCommandList* cmdList,
		ID3D12PipelineState* horzBlurPSO,
		ID3D12PipelineState* vertBlurPSO,
		ID3D12Resource* input, D3D12_RESOURCE_STATES currState,
		const INT32 blurCount);

private:
	// 블러를 수행하기 위한 가우스 가중치를 계산한다.
	std::vector<float> CalcGaussWeights(const float sigma);
	
	// 리소스를 생성한다.
	void BuildResources(ID3D12Device* device);

private:
	// 최대 블러 반지름이다. 
	// 현재 블러 반지름은 이를 넘을 수 없다.
	static inline const INT32 maxBlurRadius = 5;

	// 블러를 사용하기 위한 속성이다.
	std::vector<float> weights;
	INT32 blurRadius;

	// 블러를 그리기 위한 다이렉트 속성이다.
	UINT32 width = 0;
	UINT32 height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// 블러를 하기 위해 핑퐁(ping-pong) 수행 방식으로
	// 두 개의 리소스를 사용한다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE blur0CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE blur0CpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE blur0GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE blur0GpuUav;

	CD3DX12_CPU_DESCRIPTOR_HANDLE blur1CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE blur1CpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE blur1GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE blur1GpuUav;
	
	// 블러를 수행하는 리소스이다.
	Microsoft::WRL::ComPtr<ID3D12Resource> blurMap0 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> blurMap1 = nullptr;
};
