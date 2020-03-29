#pragma once
//***************************************************************************************
// Ssao.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include <Framework/D3DUtil.h>
#include <vector>

#define SAMPLE_COUNT 14
#define RANDOM_VECTORMAP_SIZE 256

/*
��ü���� ���� ����ϴ� ssao ���� ĸ��ȭ�� Ŭ����
*/
class Ssao
{
public:
	Ssao(ID3D12Device* device,ID3D12GraphicsCommandList* cmdList, const UINT32 width, const UINT32 height);
	Ssao(const Ssao& rhs) = delete;
	Ssao& operator=(const Ssao& rhs) = delete;
	~Ssao() = default;

public:
	// ���ϱ� ���� ���콺 ����ġ�� ����Ѵ�.
	std::vector<float> CalcGaussWeights(const float sigma) const;

	// ������ �ڵ��� �����ϰ� ������ �並 �����Ѵ�.
	void BuildDescriptors(ID3D12Device* device,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hDiffuseMapGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

	// ������ �並 ������Ѵ�.
	void RebuildDescriptors(ID3D12Device* device);
	// ���ο� �ػ󵵷� �ٲٰ� ���ҽ��� �ٽ� �����Ѵ�.
	void OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight);

	void ComputeSsao(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* ssaoPso,
		D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress);
	void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* blurPso, const INT32 blurCount);

	void GetOffsetVectors(XMFLOAT4* offsets);

	UINT32 GetWidth() const;
	UINT32 GetHeight() const;

private:
	// ssao���� ����Ų��.
	void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur);

	void BuildResources(ID3D12Device* device);
	void BuildRandomVectorTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void BuildOffsetVectors();

public:
	static inline const DXGI_FORMAT ambientMapFormat = DXGI_FORMAT_R16_UNORM;
	// �� �������� �� �ִ� �������� ���� �� ����.
	static inline const INT32 maxBlurRadius = 5;


private:
	// ssao�ʿ� �ʿ��� ���ҽ����̴�.
	Microsoft::WRL::ComPtr<ID3D12Resource> randomVectorMap;
	Microsoft::WRL::ComPtr<ID3D12Resource> randomVectorMapUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> ambientMap0;
	Microsoft::WRL::ComPtr<ID3D12Resource> ambientMap1;

	// ssao���� �ʿ��� ��ָʰ� ��ǻ�� ���� �ڵ��̴�.
	CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv;

	// ssao���� �ʿ��� ���� ���͸� ������ �� �ִ� ������ �ڵ��̴�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE hRandomVectorMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hRandomVectorMapGpuSrv;

	// ���ϴ� ���� ����(ping-ponging)�� ���� �� ���� ���ҽ��� �ʿ��ϴ�.
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

	// ssao���� �ش� �������� �ٸ� �������� �̵���Ű�� �������̴�.
	XMFLOAT4 offsets[SAMPLE_COUNT];

	const XMFLOAT4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };
};