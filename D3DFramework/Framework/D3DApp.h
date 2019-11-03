#pragma once

#include "WinApp.h"
#include "Global.h"

class D3DApp : public WinApp
{
public:
	D3DApp(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:
	virtual bool Initialize();
	virtual bool InitDirect3D();
	virtual void OnDestroy() override;

	bool Get4xMsaaState()const;
	void Set4xMsaaState(bool value);
	void SetFullscreenState(bool value);

	void SetGamma(float gamma);

protected:
	virtual void OnResize(int screenWidth, int screenHeight) override;

protected:
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetMsaaBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	virtual void CreateRtvAndDsvDescriptorHeaps();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:
	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	int mCurrentBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SWAP_CHAIN_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D32_FLOAT; // ���ٽ� ���� ��� ����
	DirectX::XMFLOAT4 mBackBufferClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
};

