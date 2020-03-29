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
�ؽ�ó�� �帮�� ���鵵�� �ϴ� ĸ��ȭ�� Ŭ����
*/
class BlurFilter
{
public:
	BlurFilter(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight, const DXGI_FORMAT newFormat);

	BlurFilter(const BlurFilter& rhs) = delete;
	BlurFilter& operator=(const BlurFilter& rhs) = delete;
	~BlurFilter() = default;

public:
	// ���� ������ �� �ϼ��� ���ҽ��� ��ȯ�Ѵ�.
	ID3D12Resource* Output();

	// ���ο� �ػ󵵷� ���ҽ��� ������Ѵ�.
	void OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight);

	// �ʿ��� ���ҽ� �並 �����ϰ� ������ �ڵ��� �����Ѵ�.
	void BuildDescriptors(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor);

	// ���� �����Ѵ�.
	void Execute(ID3D12GraphicsCommandList* cmdList,
		ID3D12PipelineState* horzBlurPSO,
		ID3D12PipelineState* vertBlurPSO,
		ID3D12Resource* input, D3D12_RESOURCE_STATES currState,
		const INT32 blurCount);

private:
	// ���� �����ϱ� ���� ���콺 ����ġ�� ����Ѵ�.
	std::vector<float> CalcGaussWeights(const float sigma);
	
	// ���ҽ��� �����Ѵ�.
	void BuildResources(ID3D12Device* device);

private:
	// �ִ� �� �������̴�. 
	// ���� �� �������� �̸� ���� �� ����.
	static inline const INT32 maxBlurRadius = 5;

	// ���� ����ϱ� ���� �Ӽ��̴�.
	std::vector<float> weights;
	INT32 blurRadius;

	// ���� �׸��� ���� ���̷�Ʈ �Ӽ��̴�.
	UINT32 width = 0;
	UINT32 height = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// ���� �ϱ� ���� ����(ping-pong) ���� �������
	// �� ���� ���ҽ��� ����Ѵ�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE blur0CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE blur0CpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE blur0GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE blur0GpuUav;

	CD3DX12_CPU_DESCRIPTOR_HANDLE blur1CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE blur1CpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE blur1GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE blur1GpuUav;
	
	// ���� �����ϴ� ���ҽ��̴�.
	Microsoft::WRL::ComPtr<ID3D12Resource> blurMap0 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> blurMap1 = nullptr;
};
