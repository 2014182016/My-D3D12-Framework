#pragma once

#include "WinApp.h"

#define ROOT_PARAMETER_NUM 10
#define RP_OBJECT 0
#define RP_PASS 1
#define RP_LIGHT 2
#define RP_MATERIAL 3
#define RP_TEXTURE 4
#define RP_SHADOWMAP 5
#define RP_CUBEMAP 6
#define RP_G_BUFFER 7
#define RP_WIDGET 8
#define RP_PARTICLE 9

#define DEFERRED_BUFFER_COUNT 4
#define LIGHT_NUM 1

class D3DApp : public WinApp
{
public:
	struct RootParameterInfo
	{
		RootParameterInfo(UINT shaderRegister, UINT registerSpace)
			: mShaderRegister(shaderRegister), mRegisterSpace(registerSpace) { }
		UINT mShaderRegister;
		UINT mRegisterSpace;
	};

public:
	D3DApp(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:
	virtual bool Initialize();
	virtual bool InitDirect3D();

	virtual void OnDestroy() override;
	virtual void OnResize(int screenWidth, int screenHeight) override;
	virtual void ApplyOption(Option option);

public:
	void Set4xMsaaState(bool value);
	void SetFullscreenState(bool value);
	void SetGamma(float gamma);

	inline DirectX::XMFLOAT4 GetBackgroundColor() const { return mBackBufferClearColor; }
	inline void SetBackgroundColor(DirectX::XMFLOAT4 color) { mBackBufferClearColor = color; }
	void SetBackgroundColor(float r, float g, float b, float a);

protected:
	void CreateDevice();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateSoundBuffer();
	void CreateRtvAndDsvDescriptorHeaps(UINT shadowMapNum);
	void CreateRootSignature(UINT textureNum, UINT cubeTextureNum, UINT shadowMapNum);
	void CreateDescriptorHeaps(UINT textureNum, UINT cubeTextureNum, UINT shadowMapNum);
	void CreateShadersAndInputLayout();
	void CreatePSOs();

	void FlushCommandQueue();

	ID3D12Resource* GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDefferedBufferView(UINT index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetCbvSrvUavDescriptorHandle(UINT index) const;

private:
	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

protected:
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
	Microsoft::WRL::ComPtr<ID3D12Resource> mDeferredBuffer[DEFERRED_BUFFER_COUNT];

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	Microsoft::WRL::ComPtr<IDirectSound8> md3dSound;
	Microsoft::WRL::ComPtr<IDirectSoundBuffer> mPrimarySoundBuffer;
	Microsoft::WRL::ComPtr<IDirectSound3DListener8> mListener;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mDefaultLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mBillboardLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mWidgetLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mLineLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mParticleLayout;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT objCBByteSize = 0;
	UINT passCBByteSize = 0;
	UINT widgetCBByteSize = 0;
	UINT particleCBByteSize = 0;

	UINT mCurrentSkyCubeMapIndex = 0;
	UINT mSkyCubeMapHeapIndex = 0;
	UINT mShadowMapHeapIndex = 0;
	UINT mDeferredBufferHeapIndex = 0;
	UINT mLightingPassHeapIndex = 0;

	UINT m4xMsaaQuality = 0;
	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	DirectX::XMFLOAT4 mBackBufferClearColor = (DirectX::XMFLOAT4)DirectX::Colors::LightSteelBlue;

private:
	const std::array<RootParameterInfo, ROOT_PARAMETER_NUM> mRootParameterInfos =
	{
		RootParameterInfo(0,0), // Object
		RootParameterInfo(1,0), // Pass
		RootParameterInfo(0,3), // Lights
		RootParameterInfo(1,3), // Materials
		RootParameterInfo(0,0), // Textures
		RootParameterInfo(0,1), // ShadowMaps
		RootParameterInfo(0,2), // CubeMaps
		RootParameterInfo(0,4), // G-Buffer
		RootParameterInfo(2,0), // Widget
		RootParameterInfo(3,0), // Particle
	};

	const std::array<DXGI_FORMAT, DEFERRED_BUFFER_COUNT> mDeferredBufferFormats =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM, // Diffuse
		DXGI_FORMAT_R8G8B8A8_UNORM, // Specular And Roughness
		DXGI_FORMAT_R32G32B32A32_FLOAT, // Normal
		DXGI_FORMAT_R32G32B32A32_FLOAT, // Position
	};

	const D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	const DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	const DXGI_FORMAT mShadowMapFormat = DXGI_FORMAT_R24G8_TYPELESS;
};

