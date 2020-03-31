#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

#include "WinApp.h"
#include "D3DInfo.h"
#include "D3DStructure.h"
#include "Defines.h"
#include "Enumeration.h"
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <dsound.h>
#include <bitset>
#include <array>
#include <wrl.h>
#include <vector>
#include <unordered_map>

class D3DApp : public WinApp
{
public:
	D3DApp(HINSTANCE hInstance, const INT32 screenWidth, const INT32 screenHeight, 
		const std::wstring applicationName, const bool useWinApi = true);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:
	virtual bool Initialize();
	virtual bool InitDirect3D();

	virtual void Tick(float deltaTime) override;
	virtual void OnDestroy() override;
	virtual void OnResize(const INT32 screenWidth, const INT32 screenHeight) override;
	virtual void CreateDescriptorHeaps(const UINT32 textureNum, const UINT32 shadowMapNum, const UINT32 particleNum);

	// 설정에 적용된 값을 프로그램에 반영한다.
	virtual void ApplyOption(const Option option);

public:
	void SetBackgroundColor(const float r, const float g, const float b, const float a);

	// 설정 값을 바꾼다. true->false, false->true가 된다.
	void SwitchOptionEnabled(const Option option);
	// 설정은 해당 value 값으로 바꾼다.
	void SetOptionEnabled(const Option option, const  bool value);
	// 설정이 켜져 있는지 여부를 반환한다.
	bool GetOptionEnabled(const Option option);

protected:
	void CreateDevice();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateSoundBuffer();
	void CreateRtvAndDsvDescriptorHeaps(const UINT32 shadowMapNum);
	void CreateRootSignatures(const UINT32 textureNum, const UINT32 shadowMapNum);
	void CreateShadersAndInputLayout();
	void CreatePSOs();

	// 명령어 리스트와 할당자를 리셋한다.
	void Reset(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc);
	// gpu 타임라인 상에서 해당 지점까지 도달하였는 지 확인한다.
	void FlushCommandQueue();

protected:
	ID3D12Resource* GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDefferedBufferView(const UINT32 index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	// 각 리소스 뷰의 인덱스 값을 통해 서술자 핸들을 돌려준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(const UINT32 index) const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(const UINT32 index) const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(const UINT32 index) const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(const UINT32 index) const;

private:
	// 해당 컴퓨터의 속성을 알기 위한 디버그 함수
	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, const DXGI_FORMAT format);

protected:
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	UINT64 currentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mainCommandAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mainCommandList;

	UINT32 currentBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffer[SWAP_CHAIN_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> deferredBuffer[DEFERRED_BUFFER_COUNT];

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvSrvUavDescriptorHeap;

	Microsoft::WRL::ComPtr<IDirectSound8> d3dSound;
	Microsoft::WRL::ComPtr<IDirectSoundBuffer> primarySoundBuffer;
	Microsoft::WRL::ComPtr<IDirectSound3DListener8> listener;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatures;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateObjects;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> shaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> defaultLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> billboardLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> widgetLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> lineLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> terrainLayout;

	D3D12_VIEWPORT screenViewport;
	D3D12_RECT scissorRect;
	UINT32 msaaQuality = 0;

	XMFLOAT4 backBufferClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	// 각 G버퍼를 지우기 위한 컬러
	const std::array<DirectX::XMFLOAT4, DEFERRED_BUFFER_COUNT> deferredBufferClearColors =
	{
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), // Diffuse, 알파 값은 라이팅의 체크여부를 다룬다
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), // Specular Roughness
		XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), // Position, 알파 값은 SSAO 및 SSR 체크 여부를 다룬다.
		XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), // Normal
		XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), // Normal(Normal Map x)
	};

	// option들의 bool값을 관리하는 컨테이너
	std::bitset<(int)Option::Count> options;

private:
	// 각 G버퍼의 포맷
	const std::array<DXGI_FORMAT, DEFERRED_BUFFER_COUNT> deferredBufferFormats =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM, // Diffuse
		DXGI_FORMAT_R8G8B8A8_UNORM, // Specular Roughness
		DXGI_FORMAT_R32G32B32A32_FLOAT, // Position
		DXGI_FORMAT_R32G32B32A32_FLOAT, // Normal
		DXGI_FORMAT_R32G32B32A32_FLOAT, // Normal(Normal Map x)
	};

	const D3D_DRIVER_TYPE d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	const DXGI_FORMAT shadowMapFormat = DXGI_FORMAT_R24G8_TYPELESS;
};

