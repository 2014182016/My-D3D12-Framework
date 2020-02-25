#include "pch.h"
#include "D3DApp.h"
#include "D3DUtil.h"
#include "Structures.h"
#include "AssetManager.h"
#include "Ssao.h"

using Microsoft::WRL::ComPtr;

D3DApp::D3DApp(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName)
	: WinApp(hInstance, screenWidth, screenHeight, applicationName)
{
	objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	widgetCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(WidgetConstants));
	particleCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ParticleConstants));
}

D3DApp::~D3DApp()
{
	mdxgiFactory = nullptr;
	mSwapChain = nullptr;
	md3dDevice = nullptr;

	mFence = nullptr;
	mCommandQueue = nullptr;
	mMainCommandAlloc = nullptr;
	mMainCommandList = nullptr;

	mDepthStencilBuffer = nullptr;
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		mSwapChainBuffer[i] = nullptr;
	for (int i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
		mDeferredBuffer[i] = nullptr;

	mRtvHeap = nullptr;
	mDsvHeap = nullptr;
	mCbvSrvUavDescriptorHeap = nullptr;

	md3dSound = nullptr;
	mPrimarySoundBuffer = nullptr;
	mListener = nullptr;

	mRootSignatures.clear();
	mPSOs.clear();
	mShaders.clear();
}


void D3DApp::Set4xMsaaState(bool value)
{
	mOptions.set((int)Option::Msaa, value);

	OnResize(mScreenWidth, mScreenHeight);
}

void D3DApp::SetFullscreenState(bool value)
{
	mOptions.set((int)Option::Fullscreen, value);

	ThrowIfFailed(mSwapChain->SetFullscreenState(value, nullptr));
	OnResize(mScreenWidth, mScreenHeight);
}

bool D3DApp::Initialize()
{
	if (!__super::Initialize())
		return false;

	if (!InitDirect3D())
		return false;

	OnResize(mScreenWidth, mScreenHeight);

	return true;
}

void D3DApp::OnDestroy()
{
	__super::OnDestroy();

	if (md3dDevice != nullptr)
		FlushCommandQueue();

	mSwapChain->SetFullscreenState(false, nullptr);
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps(UINT shadowMapNum)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
#ifdef SSAO
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT + DEFERRED_BUFFER_COUNT + 2;
#else
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT + DEFERRED_BUFFER_COUNT;
#endif
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1 + shadowMapNum;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::OnResize(int screenWidth, int screenHeight)
{
	__super::OnResize(screenWidth, screenHeight);

	if (!md3dDevice || !mSwapChain || !mMainCommandAlloc)
		return;

	/*
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);
	*/

	// ���ҽ��� �����ϱ� ���� ��ɵ��� ����.
	FlushCommandQueue();

	ThrowIfFailed(mMainCommandList->Reset(mMainCommandAlloc.Get(), nullptr));

	// ���ҽ��� ���� ����� ���� ������ ���ҽ��� �����Ѵ�.
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		mSwapChainBuffer[i].Reset();
	for (int i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
		mDeferredBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	// SwapChain�� Resize�Ѵ�.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SWAP_CHAIN_BUFFER_COUNT,
		mScreenWidth, mScreenHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrentBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	D3D12_RESOURCE_DESC defferedBufferDesc;
	ZeroMemory(&defferedBufferDesc, sizeof(defferedBufferDesc));
	defferedBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	defferedBufferDesc.Alignment = 0;
	defferedBufferDesc.SampleDesc.Count = GetOptionEnabled(Option::Msaa) ? 4 : 1;
	defferedBufferDesc.SampleDesc.Quality = GetOptionEnabled(Option::Msaa) ? (m4xMsaaQuality - 1) : 0;
	defferedBufferDesc.MipLevels = 1;
	defferedBufferDesc.DepthOrArraySize = 1;
	defferedBufferDesc.Width = mScreenWidth;
	defferedBufferDesc.Height = mScreenHeight;
	defferedBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	defferedBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_RENDER_TARGET_VIEW_DESC defferedBufferRtvDesc;
	ZeroMemory(&defferedBufferRtvDesc, sizeof(defferedBufferRtvDesc));
	defferedBufferRtvDesc.Texture2D.MipSlice = 0;
	defferedBufferRtvDesc.Texture2D.PlaneSlice = 0;
	defferedBufferRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	for (UINT i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
	{
		CD3DX12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(mDeferredBufferFormats[i], (float*)&mDeferredBufferClearColors[i]);
		defferedBufferDesc.Format = mDeferredBufferFormats[i];
		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&defferedBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(mDeferredBuffer[i].GetAddressOf())));
		md3dDevice->CreateRenderTargetView(mDeferredBuffer[i].Get(), &defferedBufferRtvDesc, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	D3D12_RESOURCE_DESC depthStencilDesc = defferedBufferDesc;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = GetOptionEnabled(Option::Msaa) ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = GetOptionEnabled(Option::Msaa) ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.ViewDimension = GetOptionEnabled(Option::Fullscreen) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

	// ����⿡ ����ȭ�� ���� �����Ѵ�. ����ȭ�� ����� ����
	// �����ϴ� ����� ȣ���� �������� �ʴ� ȣ�⺸�� ���� �� �ִ�.
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, GetDepthStencilView());

	// ���� ���۷μ� ����ϱ� ���� ���� ������ �Ѵ�.
	mMainCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ));

	// Resize ����� �����Ѵ�.
	ThrowIfFailed(mMainCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mMainCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Resize�� �Ϸ�� ������ ��ٸ���.
	FlushCommandQueue();

	// Viewport�� ScissorRect�� ������Ʈ�Ѵ�.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mScreenWidth);
	mScreenViewport.Height = static_cast<float>(mScreenHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mScreenWidth, mScreenHeight };
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// D3D12 ����� ���� Ȱ��ȭ�Ѵ�.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	CreateDevice();
	CreateCommandObjects();
	CreateSwapChain();
	CreateSoundBuffer();
	CreateRtvAndDsvDescriptorHeaps(LIGHT_NUM);
	CreateShadersAndInputLayout();

	// Alt-Enter�� ��Ȱ��ȭ�Ѵ�.
	mdxgiFactory->MakeWindowAssociation(mhMainWnd, DXGI_MWA_NO_ALT_ENTER);

#ifdef _DEBUG
	LogAdapters();
#endif

	return true;
}

void D3DApp::CreateDevice()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// D3D12Device�� ����� �Ϳ� �����Ͽ��ٸ� �ϵ���� �׷��� ��ɼ���
	// �䳻���� WARP(����Ʈ���� ���÷��� �����)�� �����Ѵ�.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	// �������� ũ��� CPU ���� �ٸ� �� �����Ƿ� �̸� �˾Ƴ��д�.
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// �ĸ� ���۸� ���� 4X MSAA ���� ���θ� �����Ѵ�.
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
}

void D3DApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mMainCommandAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mMainCommandAlloc.Get(),
		nullptr,   
		IID_PPV_ARGS(mMainCommandList.GetAddressOf())));

	// ���� ���·� �����Ѵ�. ���� ��� ����� ó�� ������ �� Reset�� ȣ���ϴµ�,
	// Reset�� ȣ���Ϸ��� CommandList�� ���� �־�� �ϱ� �����̴�.
	mMainCommandList->Close();
}

void D3DApp::CreateSwapChain()
{
	// SwapChain�� �����ϱ� ���� Fullscreen��带 �����ؾ߸� �Ѵ�.
	if (mSwapChain)
		mSwapChain->SetFullscreenState(false, nullptr);

	// SwapChain�� ����� ���� ���� SwapChain�� �����Ѵ�.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mScreenWidth;
	sd.BufferDesc.Height = mScreenHeight;
	sd.BufferDesc.RefreshRate.Numerator = 0; // ������� ����
	sd.BufferDesc.RefreshRate.Denominator = 0; // ������� ����
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1; // SwapChain���� ����ǥ��ȭ Ȱ��ȭ���� ����
	sd.SampleDesc.Quality = 0; // SwapChain���� ����ǥ��ȭ Ȱ��ȭ���� ����
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = !GetOptionEnabled(Option::Fullscreen);
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// ���� : ��ȯ �罽�� Command Queue�� �̿��ؼ� Flush�� �����Ѵ�.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void D3DApp::CreateSoundBuffer()
{
	// Direct Sound ����
	ThrowIfFailed(DirectSoundCreate8(NULL, md3dSound.GetAddressOf(), NULL));
	ThrowIfFailed(md3dSound->SetCooperativeLevel(mhMainWnd, DSSCL_PRIORITY));

	DSBUFFERDESC soundBufferDesc;
	soundBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	soundBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRL3D;
	soundBufferDesc.dwBufferBytes = 0;
	soundBufferDesc.dwReserved = 0;
	soundBufferDesc.lpwfxFormat = NULL;
	soundBufferDesc.guid3DAlgorithm = GUID_NULL;
	// �� ���� ���� ����
	ThrowIfFailed(md3dSound->CreateSoundBuffer(&soundBufferDesc, mPrimarySoundBuffer.GetAddressOf(), NULL));

	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = 44100;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nChannels = 2;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;
	ThrowIfFailed(mPrimarySoundBuffer->SetFormat(&waveFormat));
	// ������ ����
	ThrowIfFailed(mPrimarySoundBuffer->QueryInterface(IID_IDirectSound3DListener8, (void**)mListener.GetAddressOf()));
}

void D3DApp::FlushCommandQueue()
{
	// ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϵ��� ��Ÿ�� ���� ������Ų��.
	mCurrentFence++;

	// �� ��Ÿ�� ������ �����ϴ� ���(Signal)�� ��� ��⿭�� �߰��Ѵ�.
	// �� ��Ÿ�� ������ GPU�� Signal() ��ɱ����� ��� ����� ó���ϱ�
	// �������� �������� �ʴ´�.
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// GPU�� �� ��Ÿ�� ���������� ��ɵ��� �Ϸ��� ������ ��ٸ���.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// GPU�� ���� ��Ÿ�� ������ ���������� �̺�Ʈ�� �ߵ��Ѵ�.
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// GPU�� ���� ��Ÿ�� ������ ���������� ���ϴ� �̺�Ʈ�� ��ٸ���.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::GetCurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetCurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrentBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDefferedBufferView(UINT index) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		SWAP_CHAIN_BUFFER_COUNT + index,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3DApp::GetCpuSrv(int index) const
{
	auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE D3DApp::GetGpuSrv(int index) const
{
	auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDsv(int index) const
{
	auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	dsv.Offset(index, mDsvDescriptorSize);
	return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3DApp::GetRtv(int index) const
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.Offset(index, mRtvDescriptorSize);
	return rtv;
}


void D3DApp::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// nullptr�� �����ϸ� ����Ʈ�� ������ �����Ѵ�.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}

void D3DApp::CreateRootSignatures(UINT textureNum, UINT shadowMapNum)
{
	// �Ϲ������� ���̴� ���α׷��� Ư�� �ڿ���(��� ����, �ؽ�ó, ǥ������� ��)��
	// �Էµȴٰ� ����Ѵ�. ��Ʈ ������ ���̴� ���α׷��� ����ϴ� �ڿ����� �����Ѵ�.

	std::array<CD3DX12_ROOT_PARAMETER, ROOT_PARAMETER_NUM> slotRootParameter;

	CD3DX12_DESCRIPTOR_RANGE texTable[4];
	texTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, textureNum, mRootParameterInfos[RP_TEXTURE].mShaderRegister, mRootParameterInfos[RP_TEXTURE].mRegisterSpace);
	texTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shadowMapNum, mRootParameterInfos[RP_SHADOWMAP].mShaderRegister, mRootParameterInfos[RP_SHADOWMAP].mRegisterSpace);
	texTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, mRootParameterInfos[RP_SSAOMAP].mShaderRegister, mRootParameterInfos[RP_SSAOMAP].mRegisterSpace);
	texTable[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, DEFERRED_BUFFER_COUNT + 1, mRootParameterInfos[RP_G_BUFFER].mShaderRegister, mRootParameterInfos[RP_G_BUFFER].mRegisterSpace);

	// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
	slotRootParameter[RP_OBJECT].InitAsConstantBufferView(mRootParameterInfos[RP_OBJECT].mShaderRegister, mRootParameterInfos[RP_OBJECT].mRegisterSpace); // Object ��� ����
	slotRootParameter[RP_PASS].InitAsConstantBufferView(mRootParameterInfos[RP_PASS].mShaderRegister, mRootParameterInfos[RP_PASS].mRegisterSpace); // Pass ��� ����
	slotRootParameter[RP_LIGHT].InitAsShaderResourceView(mRootParameterInfos[RP_LIGHT].mShaderRegister, mRootParameterInfos[RP_LIGHT].mRegisterSpace); // Lights
	slotRootParameter[RP_MATERIAL].InitAsShaderResourceView(mRootParameterInfos[RP_MATERIAL].mShaderRegister, mRootParameterInfos[RP_MATERIAL].mRegisterSpace); // Materials
	slotRootParameter[RP_TEXTURE].InitAsDescriptorTable(1, &texTable[0], D3D12_SHADER_VISIBILITY_PIXEL); // Textures
	slotRootParameter[RP_SHADOWMAP].InitAsDescriptorTable(1, &texTable[1], D3D12_SHADER_VISIBILITY_PIXEL); // Shadow Maps
	slotRootParameter[RP_SSAOMAP].InitAsDescriptorTable(1, &texTable[2], D3D12_SHADER_VISIBILITY_PIXEL); // Ssao Maps
	slotRootParameter[RP_G_BUFFER].InitAsDescriptorTable(1, &texTable[3], D3D12_SHADER_VISIBILITY_PIXEL); // G-Buffer
	slotRootParameter[RP_WIDGET].InitAsConstantBufferView(mRootParameterInfos[RP_WIDGET].mShaderRegister, mRootParameterInfos[RP_WIDGET].mRegisterSpace); // Widget
	slotRootParameter[RP_PARTICLE].InitAsConstantBufferView(mRootParameterInfos[RP_PARTICLE].mShaderRegister, mRootParameterInfos[RP_PARTICLE].mRegisterSpace); // Particle
	slotRootParameter[RP_SSAO].InitAsConstantBufferView(mRootParameterInfos[RP_SSAO].mShaderRegister, mRootParameterInfos[RP_SSAO].mRegisterSpace); // Ssao
	slotRootParameter[RP_BLUR].InitAsConstants(1, mRootParameterInfos[RP_BLUR].mShaderRegister); // Blur

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((UINT)slotRootParameter.size(), slotRootParameter.data(),
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Root Signature�� ����ȭ�� �� ��ü�� �����Ѵ�.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignatures["Common"])));

	/*
#ifdef SSAO
	CD3DX12_DESCRIPTOR_RANGE ssaoTexTable[3];
	ssaoTexTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	ssaoTexTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	ssaoTexTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	std::array<CD3DX12_ROOT_PARAMETER, 5> ssaoSlotRootParameter;
	ssaoSlotRootParameter[0].InitAsConstantBufferView(0);
	ssaoSlotRootParameter[1].InitAsConstants(1, 1);
	ssaoSlotRootParameter[2].InitAsDescriptorTable(1, &ssaoTexTable[0], D3D12_SHADER_VISIBILITY_PIXEL);
	ssaoSlotRootParameter[3].InitAsDescriptorTable(1, &ssaoTexTable[1], D3D12_SHADER_VISIBILITY_PIXEL);
	ssaoSlotRootParameter[4].InitAsDescriptorTable(1, &ssaoTexTable[2], D3D12_SHADER_VISIBILITY_PIXEL);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> ssaoStaticSamplers =
	{
		pointClamp, linearClamp, depthMapSam, linearWrap
	};

	CD3DX12_ROOT_SIGNATURE_DESC ssaoRootSigDesc((UINT)ssaoSlotRootParameter.size(), ssaoSlotRootParameter.data(),
		(UINT)ssaoStaticSamplers.size(), ssaoStaticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	serializedRootSig = nullptr;
	errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&ssaoRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignatures["Ssao"])));
#endif
	*/
}


void D3DApp::CreateDescriptorHeaps(UINT textureNum, UINT shadowMapNum)
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavDescriptorHeap = {};
	cbvSrvUavDescriptorHeap.NumDescriptors = textureNum +  shadowMapNum + DEFERRED_BUFFER_COUNT + 1 + 3; // Depth Buffer, SsaoMaps
	cbvSrvUavDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvUavDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvSrvUavDescriptorHeap, IID_PPV_ARGS(&mCbvSrvUavDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// �ؽ�ó�� Shader Resource View�� �����ϴ� ����ü
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// �� �ؽ�ó���� ���� Shader Resource View�� �����Ѵ�.
	for (UINT i = 0; i < textureNum; ++i)
	{
		// texNames�� �ε����� Material�� ���� �ؽ�ó �ε����� �����ֱ� ���� ������� View�� �����Ѵ�.
		ID3D12Resource* resource = AssetManager::GetInstance()->GetTextureResource(i);

		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource, &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	mDeferredBufferHeapIndex = textureNum;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	for (UINT i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
	{
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = mDeferredBufferFormats[i];

		// G-Buffer�� ���Ǵ� ���� Ÿ�ٵ鿡 ���� SRV�� �����Ѵ�.
		md3dDevice->CreateShaderResourceView(mDeferredBuffer[i].Get(), &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	// G-Buffer�� ���Ǵ� ���� ���ۿ� ���� SRV�� �����Ѵ�.
	md3dDevice->CreateShaderResourceView(mDepthStencilBuffer.Get(), &srvDesc, hDescriptor);
	hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
}

void D3DApp::CreateShadersAndInputLayout()
{
	// Direct3D���� �� ������ ������ �˷��ִ� InputLayout��
	// ���̴��� �������Ѵ�.

#ifdef SSAO
	D3D_SHADER_MACRO defines[] = {
		"SSAO", "1",
		NULL, NULL,
	};
#else
	D3D_SHADER_MACRO defines[] = {
		NULL, NULL,
	};
#endif

	mShaders["ForwardVS"] = D3DUtil::CompileShader(L"Shaders\\Forward.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ForwardPS"] = D3DUtil::CompileShader(L"Shaders\\Forward.hlsl", defines, "PS", "ps_5_1");

	mShaders["OpaqueVS"] = D3DUtil::CompileShader(L"Shaders\\Opaque.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["OpaquePS"] = D3DUtil::CompileShader(L"Shaders\\Opaque.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["WireframeVS"] = D3DUtil::CompileShader(L"Shaders\\Wireframe.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["WireframePS"] = D3DUtil::CompileShader(L"Shaders\\Wireframe.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SkyVS"] = D3DUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkyPS"] = D3DUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ShadowMapVS"] = D3DUtil::CompileShader(L"Shaders\\ShadowMap.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ShadowMapPS"] = D3DUtil::CompileShader(L"Shaders\\ShadowMap.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["WidgetVS"] = D3DUtil::CompileShader(L"Shaders\\Widget.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["WidgetPS"] = D3DUtil::CompileShader(L"Shaders\\Widget.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ParticleVS"] = D3DUtil::CompileShader(L"Shaders\\Particle.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ParticleGS"] = D3DUtil::CompileShader(L"Shaders\\Particle.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["ParticlePS"] = D3DUtil::CompileShader(L"Shaders\\Particle.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["BillboardVS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["BillboardGS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["BillboardPS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["DiffuseMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "DiffuseMapDebugPS", "ps_5_1");
	mShaders["SpecularMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "SpecularMapDebugPS", "ps_5_1");
	mShaders["RoughnessMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "RoughnessMapDebugPS", "ps_5_1");
	mShaders["NormalMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "NormalMapDebugPS", "ps_5_1");
	mShaders["DepthMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "DepthMapDebugPS", "ps_5_1");
	mShaders["PositionMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "PositionMapDebugPS", "ps_5_1");
	mShaders["ShadowMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "ShadowMapDebugPS", "ps_5_1");
	mShaders["SsaoMapDebugPS"] = D3DUtil::CompileShader(L"Shaders\\MapDebug.hlsl", nullptr, "SsaoMapDebugPS", "ps_5_1");

	mShaders["LightingPassPS"] = D3DUtil::CompileShader(L"Shaders\\LightingPass.hlsl", defines, "PS", "ps_5_1");

	mShaders["SsaoVS"] = D3DUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SsaoPS"] = D3DUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["SsaoBlurVS"] = D3DUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SsaoBlurPS"] = D3DUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

	mDefaultLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mBillboardLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mWidgetLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mLineLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mParticleLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void D3DApp::CreatePSOs()
{
	// Direct3D���� ����� ������������ ���´� PSO��ü�� ���� �̸� �����ȴ�.
	// �� ��� ���� �ϳ��� ����ü�� �����ϴ� ������ Direct3D�� ��� ���°�
	// ȣȯ�Ǵ��� �̸� ������ �� ������ ����̹��� �ϵ���� ������ ���α׷�����
	// ���� ��� �ڵ带 �̸� ������ �� �ֱ� �����̴�.

	// PSO for Forward
	D3D12_GRAPHICS_PIPELINE_STATE_DESC forwardPsoDesc;
	ZeroMemory(&forwardPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	forwardPsoDesc.InputLayout = { mDefaultLayout.data(), (UINT)mDefaultLayout.size() };
	forwardPsoDesc.pRootSignature = mRootSignatures["Common"].Get();
	forwardPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ForwardVS"]->GetBufferPointer()),
		mShaders["ForwardVS"]->GetBufferSize()
	};
	forwardPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ForwardPS"]->GetBufferPointer()),
		mShaders["ForwardPS"]->GetBufferSize()
	};
	forwardPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	forwardPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	forwardPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	forwardPsoDesc.SampleMask = UINT_MAX;
	forwardPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	forwardPsoDesc.NumRenderTargets = 1;
	forwardPsoDesc.RTVFormats[0] = mBackBufferFormat;
	forwardPsoDesc.SampleDesc.Count = 1;
	forwardPsoDesc.SampleDesc.Quality = 0;
	forwardPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&forwardPsoDesc, IID_PPV_ARGS(&mPSOs["Forward"])));


	// PSO for G-Buffer
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gBufferPsoDesc = forwardPsoDesc;
	gBufferPsoDesc.NumRenderTargets = DEFERRED_BUFFER_COUNT;
	for (UINT i = 0; i < DEFERRED_BUFFER_COUNT; ++i)
	{
		gBufferPsoDesc.RTVFormats[i] = mDeferredBufferFormats[i];
	}

	// PSO for Opaque
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = gBufferPsoDesc;
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["OpaqueVS"]->GetBufferPointer()),
		mShaders["OpaqueVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["OpaquePS"]->GetBufferPointer()),
		mShaders["OpaquePS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["Opaque"])));

	// PSO for Billborad
	D3D12_GRAPHICS_PIPELINE_STATE_DESC billboardPsoDesc = gBufferPsoDesc;
	billboardPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["BillboardVS"]->GetBufferPointer()),
		mShaders["BillboardVS"]->GetBufferSize()
	};
	billboardPsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["BillboardGS"]->GetBufferPointer()),
		mShaders["BillboardGS"]->GetBufferSize()
	};
	billboardPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["BillboardPS"]->GetBufferPointer()),
		mShaders["BillboardPS"]->GetBufferSize()
	};
	billboardPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	billboardPsoDesc.InputLayout = { mBillboardLayout.data(), (UINT)mBillboardLayout.size() };
	billboardPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// MSAA�� Ȱ��ȭ�Ǿ� �ִٸ� �ϵ����� ���� ���� �̿��ؼ� MSAA�� ���� ���� �����Ͽ� ���ø��Ѵ�.
	billboardPsoDesc.BlendState.AlphaToCoverageEnable = GetOptionEnabled(Option::Msaa);
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&billboardPsoDesc, IID_PPV_ARGS(&mPSOs["Billborad"])));


	// PSO for Particle
	D3D12_GRAPHICS_PIPELINE_STATE_DESC particlePsoDesc = forwardPsoDesc;
	particlePsoDesc.InputLayout = { mParticleLayout.data(), (UINT)mParticleLayout.size() };
	particlePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ParticleVS"]->GetBufferPointer()),
		mShaders["ParticleVS"]->GetBufferSize()
	};
	particlePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["ParticleGS"]->GetBufferPointer()),
		mShaders["ParticleGS"]->GetBufferSize()
	};
	particlePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ParticlePS"]->GetBufferPointer()),
		mShaders["ParticlePS"]->GetBufferSize()
	};
	particlePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	particlePsoDesc.BlendState.AlphaToCoverageEnable = GetOptionEnabled(Option::Msaa);
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&particlePsoDesc, IID_PPV_ARGS(&mPSOs["Particle"])));


	// PSO for AlphaTested
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = gBufferPsoDesc;
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["AlphaTested"])));


	// PSO for Wireframe 
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = forwardPsoDesc;
	opaqueWireframePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["WireframeVS"]->GetBufferPointer()),
		mShaders["WireframeVS"]->GetBufferSize()
	};
	opaqueWireframePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["WireframePS"]->GetBufferPointer()),
		mShaders["WireframePS"]->GetBufferSize()
	};
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaqueWireframePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["Wireframe"])));


	// PSO for Transparent
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = forwardPsoDesc;
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	// �������� ��ü���� ������ �����Ǿ� ���̰� �Ϸ��� ���� ���⸦ ��Ȱ��ȭ�ϰų� �������� ��ü�� �����Ͽ� �׷��� �Ѵ�.
	// D3D12_DEPTH_WRITE_MASK_ZERO�� ���� ��ϸ� ��Ȱ��ȭ�� �� ���� �б�� ���� ������ ������ Ȱ��ȭ�Ѵ�.
	transparentPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["Transparent"])));


	// PSO for Sky
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = forwardPsoDesc;
	// �ɸ޶�� �ϴ� �� ���ο� �����Ƿ� �ĸ� ������ ����.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// ���� �Լ��� LESS�� �ƴ϶� LESS_EQUAL�� �����Ѵ�.
	// �̷��� ���� ������, ���� ���۸� 1�� ����� ��� z = 1(NDC)����
	// ����ȭ�� ���� ���� ���� ������ �����Ѵ�.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkyVS"]->GetBufferPointer()),
		mShaders["SkyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SkyPS"]->GetBufferPointer()),
		mShaders["SkyPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["Sky"])));


	// PSO for Shadow
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = forwardPsoDesc;
	// Bias = (float)DepthBias * r + SlopeScaledDepthBias * MaxDepthSlope;
	// ���⼭ r�� ���� ���� ������ float32�� ��ȯ���� ��, ǥ�� ������ 0���� ū �ּڰ��̴�.
	// 24��Ʈ ���� ������ ��� r = 1 /2^24�̴�.
	// ex. DepthBias = 100000 -> ���� ���� ����ġ = 100000 / 2^24 = 0.006
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowMapVS"]->GetBufferPointer()),
		mShaders["ShadowMapVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowMapPS"]->GetBufferPointer()),
		mShaders["ShadowMapPS"]->GetBufferSize()
	};
	// �׸��� �� �н����� ���� ����� ����.
	// �׸��� ���� ���� ���۸��� ����ϹǷ� ���� ����� ������� �������ν�
	// ������ ����ȭ�� ������ �� �ִ�.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["ShadowMap"])));

#ifdef SSAO
	// PSO for Ssao
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = forwardPsoDesc;
	ssaoPsoDesc.InputLayout = { nullptr, 0 };
	ssaoPsoDesc.pRootSignature = mRootSignatures["Common"].Get();
	ssaoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SsaoVS"]->GetBufferPointer()),
		mShaders["SsaoVS"]->GetBufferSize()
	};
	ssaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SsaoPS"]->GetBufferPointer()),
		mShaders["SsaoPS"]->GetBufferSize()
	};
	ssaoPsoDesc.DepthStencilState.DepthEnable = false;
	ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPsoDesc.RTVFormats[0] = Ssao::AmbientMapFormat;
	ssaoPsoDesc.SampleDesc.Count = 1;
	ssaoPsoDesc.SampleDesc.Quality = 0;
	ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&mPSOs["Ssao"])));

	// PSO for SsaoBlur
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPsoDesc = ssaoPsoDesc;
	ssaoBlurPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["SsaoBlurVS"]->GetBufferPointer()),
		mShaders["SsaoBlurVS"]->GetBufferSize()
	};
	ssaoBlurPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SsaoBlurPS"]->GetBufferPointer()),
		mShaders["SsaoBlurPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoBlurPsoDesc, IID_PPV_ARGS(&mPSOs["SsaoBlur"])));
#endif


	// PSO for Widget
	D3D12_GRAPHICS_PIPELINE_STATE_DESC widgetPsoDesc = forwardPsoDesc;
	widgetPsoDesc.InputLayout = { mWidgetLayout.data(), (UINT)mWidgetLayout.size() };
	widgetPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["WidgetVS"]->GetBufferPointer()),
		mShaders["WidgetVS"]->GetBufferSize()
	};
	widgetPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["WidgetPS"]->GetBufferPointer()),
		mShaders["WidgetPS"]->GetBufferSize()
	};
	widgetPsoDesc.DepthStencilState.DepthEnable = false;
	widgetPsoDesc.DepthStencilState.StencilEnable = false;
	widgetPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&widgetPsoDesc, IID_PPV_ARGS(&mPSOs["Widget"])));

	// PSO for DiffuseMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC diffuseMapDebugPsoDesc = widgetPsoDesc;
	diffuseMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["DiffuseMapDebugPS"]->GetBufferPointer()),
		mShaders["DiffuseMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&diffuseMapDebugPsoDesc, IID_PPV_ARGS(&mPSOs["DiffuseMapDebug"])));


	// PSO for SpecularMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC specularMapDebugPsoDesc = widgetPsoDesc;
	specularMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SpecularMapDebugPS"]->GetBufferPointer()),
		mShaders["SpecularMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&specularMapDebugPsoDesc, IID_PPV_ARGS(&mPSOs["SpecularMapDebug"])));


	// PSO for RoughnessMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC roughnessMapDebugDebugPsoDesc = widgetPsoDesc;
	roughnessMapDebugDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["RoughnessMapDebugPS"]->GetBufferPointer()),
		mShaders["RoughnessMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&roughnessMapDebugDebugPsoDesc, IID_PPV_ARGS(&mPSOs["RoughnessMapDebug"])));


	// PSO for NormalMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalMapDebuggPsoDesc = widgetPsoDesc;
	normalMapDebuggPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["NormalMapDebugPS"]->GetBufferPointer()),
		mShaders["NormalMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&normalMapDebuggPsoDesc, IID_PPV_ARGS(&mPSOs["NormalMapDebug"])));


	// PSO for DepthMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC depthMapDebugPsoDesc = widgetPsoDesc;
	depthMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["DepthMapDebugPS"]->GetBufferPointer()),
		mShaders["DepthMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&depthMapDebugPsoDesc, IID_PPV_ARGS(&mPSOs["DepthMapDebug"])));


	// PSO for ShadowMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowMapDebugPsoDesc = widgetPsoDesc;
	shadowMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowMapDebugPS"]->GetBufferPointer()),
		mShaders["ShadowMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowMapDebugPsoDesc, IID_PPV_ARGS(&mPSOs["ShadowMapDebug"])));


	// PSO for SsaoMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoMapDebugPsoDesc = widgetPsoDesc;
	ssaoMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["SsaoMapDebugPS"]->GetBufferPointer()),
		mShaders["SsaoMapDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoMapDebugPsoDesc, IID_PPV_ARGS(&mPSOs["SsaoMapDebug"])));


	// PSO for LightingPass
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lightingPassPsoDesc = widgetPsoDesc;
	lightingPassPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["LightingPassPS"]->GetBufferPointer()),
		mShaders["LightingPassPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&lightingPassPsoDesc, IID_PPV_ARGS(&mPSOs["LightingPass"])));
}

void D3DApp::SetGamma(float gamma)
{
	// ���� ������ Ǯ��ũ�� ��忡���� ����� �� �ִ�.
	if (!GetOptionEnabled(Option::Fullscreen))
		return;

	IDXGIOutput* output;
	DXGI_GAMMA_CONTROL_CAPABILITIES gammaCaps;
	DXGI_GAMMA_CONTROL gammaControl;

	ThrowIfFailed(mSwapChain->GetContainingOutput(&output));
	ThrowIfFailed(output->GetGammaControlCapabilities(&gammaCaps));

	gammaControl.Scale.Red = 1.0f;
	gammaControl.Scale.Green = 1.0f;
	gammaControl.Scale.Blue = 1.0f;

	gammaControl.Offset.Red = 1.0f;
	gammaControl.Offset.Green = 1.0f;
	gammaControl.Offset.Blue = 1.0f;

	for (UINT i = 0; i < gammaCaps.NumGammaControlPoints; ++i)
	{
		float p0 = gammaCaps.ControlPointPositions[i];
		float p1 = pow(p0, 1.0f / gamma); // ���� Ŀ�� ����

		gammaControl.GammaCurve[i].Red = p1;
		gammaControl.GammaCurve[i].Green = p1;
		gammaControl.GammaCurve[i].Blue = p1;
	}

	ThrowIfFailed(output->SetGammaControl(&gammaControl));
}

void D3DApp::SetBackgroundColor(float r, float g, float b, float a)
{
	mBackBufferClearColor.x = r;
	mBackBufferClearColor.y = g;
	mBackBufferClearColor.z = b;
	mBackBufferClearColor.w = a;
}

void D3DApp::ApplyOption(Option option)
{
	bool optionState = mOptions.test((int)option);

	switch (option)
	{
		case Option::Fullscreen:
			SetFullscreenState(optionState);
			break;
		case Option::Msaa:
			Set4xMsaaState(optionState);
			break;
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 8> D3DApp::GetStaticSamplers()
{
	// �׷��� ���� ���α׷��� ����ϴ� ǥ��������� ���� �׸� ���� �����Ƿ�
	// �̸� ���� Root Signature�� ���Խ��� �д�.

	// �� ���͸�, ��ȯ ��ǥ ���� ���
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	// �� ���͸�, ���� ��ǥ ���� ���
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	// ���� ���͸�, ��ȯ ��ǥ ���� ���
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	// ���� ���͸�, ���� ��ǥ ���� ���
	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	// ���� ���͸�, ��ȯ ��ǥ ���� ���
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	// ���� ���͸�, ���� ��ǥ ���� ���
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	// �׸��ڸ� ���� �� ǥ�������
	// �ϵ��� �׸��� �� �� �������� ������ �� �ְ� �ϱ� ���� ��
	// PCF(Percenatge Colser Filtering) : ���� ���� ���͸�
	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		7, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow, depthMapSam
	};
}

void D3DApp::Reset(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc)
{
	// ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� �缳���Ѵ�.
	// �缳���� GPU�� ���� ��ɸ�ϵ��� ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(cmdAlloc->Reset());

	// ��� ����� ExcuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ�
	// ��� ����� �缳���� �� �ִ�. ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(cmdList->Reset(cmdAlloc, nullptr));
}