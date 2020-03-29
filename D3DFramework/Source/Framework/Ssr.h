#pragma once

#include <Framework/D3DUtil.h>

/*
���� �������� G���۸� ����Ͽ� ��ü�� �ݻ��
����� �׸����� ĸ��ȭ�� Ŭ����
*/
class Ssr
{
public:
	Ssr(ID3D12Device* device, const UINT32 width, const UINT32 height);
	Ssr(const Ssr& rhs) = delete;
	Ssr& operator=(const Ssr& rhs) = delete;
	~Ssr() = default;

public:
	ID3D12Resource* Output();

	// ������ �ڵ��� �����ϰ�, ������ �並 �����Ѵ�.
	void BuildDescriptors(ID3D12Device* device,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

	// ���ο� �ػ󵵷� �ٲٰ�, ���ҽ��� ���� �����Ѵ�.
	void OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight);

	void ComputeSsr(ID3D12GraphicsCommandList* cmdList, CD3DX12_GPU_DESCRIPTOR_HANDLE currColorMapGpuSrv,
		D3D12_GPU_VIRTUAL_ADDRESS ssrCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress);

public:
	static const DXGI_FORMAT ssrMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
	void BuildResources(ID3D12Device* device);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> ssrMap;

	// ssr���� ����ϱ� ���� ��ġ �ʰ� ��� ���� �ڵ��̴�.
	CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv;

	// ssr���� ����� ssr ���� ������ �ڵ��̴�.
	CD3DX12_GPU_DESCRIPTOR_HANDLE hSsrMapGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hSsrMapCpuRtv;

	UINT32 renderTargetWidth;
	UINT32 renderTargetHeight;

	D3D12_VIEWPORT viewportRect;
	D3D12_RECT scissorRect;

	const XMFLOAT4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
};