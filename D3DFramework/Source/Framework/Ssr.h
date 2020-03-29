#pragma once

#include <Framework/D3DUtil.h>

/*
지연 렌더링의 G버퍼를 사용하여 객체의 반사된
모습을 그리도록 캡슐화한 클래스
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

	// 서술자 핸들을 저장하고, 서술자 뷰를 생성한다.
	void BuildDescriptors(ID3D12Device* device,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);

	// 새로운 해상도로 바꾸고, 리소스를 새로 생성한다.
	void OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight);

	void ComputeSsr(ID3D12GraphicsCommandList* cmdList, CD3DX12_GPU_DESCRIPTOR_HANDLE currColorMapGpuSrv,
		D3D12_GPU_VIRTUAL_ADDRESS ssrCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress);

public:
	static const DXGI_FORMAT ssrMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
	void BuildResources(ID3D12Device* device);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> ssrMap;

	// ssr에서 사용하기 위한 위치 맵과 노멀 맵의 핸들이다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv;

	// ssr에서 사용할 ssr 맵의 서술자 핸들이다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE hSsrMapGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hSsrMapCpuRtv;

	UINT32 renderTargetWidth;
	UINT32 renderTargetHeight;

	D3D12_VIEWPORT viewportRect;
	D3D12_RECT scissorRect;

	const XMFLOAT4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
};