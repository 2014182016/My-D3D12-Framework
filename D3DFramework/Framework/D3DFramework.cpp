#include "pch.h"
#include "D3DFramework.h"
#include "FrameResource.h"
#include "UploadBuffer.h"
#include "GameTimer.h"
#include "Camera.h"
#include "D3DUtil.h"
#include "AssetManager.h"
#include "GameObject.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

D3DFramework::D3DFramework(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName)
	: D3DApp(hInstance, screenWidth, screenHeight, applicationName) 
{
	mCamera = new Camera();
}

D3DFramework::~D3DFramework() { }


bool D3DFramework::Initialize()
{
	if (!__super::Initialize())
		return false;

	// �ʱ�ȭ ��ɵ��� �غ��ϱ� ���� ��� ��ϵ��� �缳���Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCamera->SetPosition(0.0f, 2.0f, -15.0f);

	objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	mAssetManager = new AssetManager();
	mAssetManager->Initialize(md3dDevice.Get(), mCommandList.Get());

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	// �ʱ�ȭ ��ɵ��� �����Ѵ�.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ʱ�ȭ�� ���� ������ ��ٸ���.
	FlushCommandQueue();

	// RenderLayer�� �������� Object�� �����Ѵ�.
	mAllObjects.sort(ObjectLess());
	ClassifyObjectLayer();

	for (const auto& obj : mAllObjects)
	{
		obj->BeginPlay();
	}

	return true;
}

void D3DFramework::OnDestroy()
{
	delete mCamera;
	mCamera = 0;

	__super::OnDestroy();

}

void D3DFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DFramework::OnResize(int screenWidth, int screenHeight)
{
	__super::OnResize(screenWidth, screenHeight);

	mCamera->SetLens(0.25f*DirectX::XM_PI, GetAspectRatio(), 1.0f, 1000.0f);
}

void D3DFramework::Tick(float deltaTime)
{
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % NUM_FRAME_RESOURCES;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	// GPU�� ���� FrameResource���� ����� �����ƴ��� Ȯ���Ѵ�.
	// �ƴ϶�� Fence Point�� ������ ������ ��ٸ���.
	if (mCurrentFrameResource->mFence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->mFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->mFence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	for (const auto& obj : mAllObjects)
	{
		obj->Tick(deltaTime);
	}

	AnimateMaterials(deltaTime);
	UpdateObjectCBs(deltaTime);
	UpdateMaterialBuffer(deltaTime);
	UpdateMainPassCB(deltaTime);
}

void D3DFramework::Render()
{
	auto cmdListAlloc = mCurrentFrameResource->mCmdListAlloc;

	// ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� �缳���Ѵ�.
	// �缳���� GPU�� ���� ��ɸ�ϵ��� ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(cmdListAlloc->Reset());

	// ��� ����� ExcuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ�
	// ��� ����� �缳���� �� �ִ�. ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), nullptr));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// �ĸ� ���۰� ���� ���۸� �����. 
	mCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), (float*)&mBackBufferClearColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// �׸��� ���� �ĸ� ���۰� ���� ���۸� �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvSrvUavDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// �� ��鿡 ���Ǵ� Pass ��� ���۸� ���´�.
	auto passCB = mCurrentFrameResource->mPassCB->GetResource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	// �� ��鿡�� �翵�Ǵ� ��� ���͸����� ���´�. ������ ���۴�
	// ���� �����ϰ� �׳� �ϳ��� ��Ʈ �����ڷ� ���� �� �ִ�.
	auto matBuffer = mCurrentFrameResource->mMaterialBuffer->GetResource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	// �� ��鿡 ���Ǵ� ��� �ؽ�ó�� ���´�. ���̺��� ù �����ڸ� ������
	// ���̺� �� ���� �����ڰ� �ִ����� Root Signature�� �����Ǿ� �ִ�.
	mCommandList->SetGraphicsRootDescriptorTable(3, mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	if (mIsWireframe)
	{
		mCommandList->SetPipelineState(mPSOs["Wireframe"].Get());
		RenderObjects(mCommandList.Get(), mAllObjects.begin(), mAllObjects.end());
	}
	else
	{
		mCommandList->SetPipelineState(mPSOs["Opaque"].Get());
		RenderObjects(mCommandList.Get(), mLayerPair[(int)RenderLayer::Opaque].first, 
			mLayerPair[(int)RenderLayer::Opaque].second);

		mCommandList->SetPipelineState(mPSOs["AlphaTested"].Get());
		RenderObjects(mCommandList.Get(), mLayerPair[(int)RenderLayer::AlphaTested].first, 
			mLayerPair[(int)RenderLayer::AlphaTested].second);

		mCommandList->SetPipelineState(mPSOs["Billborad"].Get());
		RenderObjects(mCommandList.Get(), mLayerPair[(int)RenderLayer::Billborad].first, 
			mLayerPair[(int)RenderLayer::Billborad].second);

		mCommandList->SetPipelineState(mPSOs["Transparent"].Get());
		RenderObjects(mCommandList.Get(), mLayerPair[(int)RenderLayer::Transparent].first,
			mLayerPair[(int)RenderLayer::Transparent].second);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// ���� ���ۿ� �ĸ� ���۸� �ٲ۴�.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

	// ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϵ��� ��Ÿ�� ���� ������Ų��.
	mCurrentFrameResource->mFence = ++mCurrentFence;

	// �� ��Ÿ�� ������ �����ϴ� ���(Signal)�� ��� ��⿭�� �߰��Ѵ�.
	// �� ��Ÿ�� ������ GPU�� �� Signal() ��ɱ����� ��� ����� ó���ϱ�
	// �������� �������� �ʴ´�.
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void D3DFramework::AnimateMaterials(float deltaTime) { }

void D3DFramework::UpdateObjectCBs(float deltaTime)
{
	auto currObjectCB = mCurrentFrameResource->mObjectCB.get();
	for (auto& obj : mAllObjects)
	{
		GameObject* gameObject = static_cast<GameObject*>(obj.get());
		if (gameObject->IsUpdate())
		{
			ObjectConstants objConstants;

			XMStoreFloat4x4(&objConstants.mWorld, XMMatrixTranspose(gameObject->GetWorld()));
			XMStoreFloat4x4(&objConstants.mTexTransform, XMMatrixTranspose(gameObject->GetTexTransform()));
			objConstants.mMaterialIndex = gameObject->GetMaterial()->mMatCBIndex;

			currObjectCB->CopyData(gameObject->GetObjectCBIndex(), objConstants);

			gameObject->DecreaseNumFrames();
		}
	}
}

void D3DFramework::UpdateMaterialBuffer(float deltaTime)
{
	auto currMaterialBuffer = mCurrentFrameResource->mMaterialBuffer.get();
	for (const auto& e : mAssetManager->GetMaterials())
	{
		Material* mat = e.second.get();
		if (mat->mNumFramesDirty > 0)
		{
			MaterialData matData;

			matData.mDiffuseAlbedo = mat->mDiffuseAlbedo;
			matData.mSpecular = mat->mSpecular;
			matData.mRoughness = mat->mRoughness;
			matData.mDiffuseMapIndex = mat->mDiffuseSrvHeapIndex;
			matData.mNormalMapIndex = mat->mNormalSrvHeapIndex;

			XMMATRIX transform = XMLoadFloat4x4(&mat->mMatTransform);
			DirectX::XMStoreFloat4x4(&matData.mMatTransform, XMMatrixTranspose(transform));

			currMaterialBuffer->CopyData(mat->mMatCBIndex, matData);

			--mat->mNumFramesDirty;
		}
	}
}

void D3DFramework::UpdateMainPassCB(float deltaTime)
{
	XMMATRIX view = mCamera->GetView();
	XMMATRIX proj = mCamera->GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.mView, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.mInvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.mProj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.mInvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.mViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.mInvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.mEyePosW = mCamera->GetPosition3f();
	mMainPassCB.mRenderTargetSize = XMFLOAT2((float)mScreenWidth, (float)mScreenHeight);
	mMainPassCB.mInvRenderTargetSize = XMFLOAT2(1.0f / mScreenWidth, 1.0f / mScreenHeight);
	mMainPassCB.mNearZ = 1.0f;
	mMainPassCB.mFarZ = 1000.0f;
	mMainPassCB.mTotalTime = mTimer.GetTotalTime();
	mMainPassCB.mDeltaTime = mTimer.GetDeltaTime();
	mMainPassCB.mAmbientLight = { 0.05f, 0.05f, 0.05f, 1.0f };
	mMainPassCB.mFogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	mMainPassCB.mFogStart = 5.0f;
	mMainPassCB.mFogRange = 10.0f;
	mMainPassCB.mLights[0].mDirection = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.mLights[0].mStrength = { 0.8f, 0.8f, 0.8f };

	auto currPassCB = mCurrentFrameResource->mPassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void D3DFramework::BuildRootSignature()
{
	// �Ϲ������� ���̴� ���α׷��� Ư�� �ڿ���(��� ����, �ؽ�ó, ǥ������� ��)��
	// �Էµȴٰ� ����Ѵ�. ��Ʈ ������ ���̴� ���α׷��� ����ϴ� �ڿ����� �����Ѵ�.

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)mAssetManager->GetTextures().size(), 0, 0);

	// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
	slotRootParameter[0].InitAsConstantBufferView(0); // Object ��� ����
	slotRootParameter[1].InitAsConstantBufferView(1); // Pass ��� ����
	slotRootParameter[2].InitAsShaderResourceView(0, 1); // Materials
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // Textures

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
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
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void D3DFramework::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavDescriptorHeap = {};
	cbvSrvUavDescriptorHeap.NumDescriptors = (UINT)mAssetManager->GetTextures().size();
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
	for (UINT i = 0; i < (UINT)mAssetManager->GetTextures().size(); ++i)
	{
		// texNames�� �ε����� Material�� ���� �ؽ�ó �ε����� �����ֱ� ���� ������� View�� �����Ѵ�.
		ID3D12Resource* resource = mAssetManager->GetTextureResource(i);

		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource, &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
}

void D3DFramework::BuildShadersAndInputLayout()
{
	// Direct3D���� �� ������ ������ �˷��ִ� InputLayout��
	// ���̴��� �������Ѵ�.

	// ���̴����� ����� define�� �����Ѵ�.
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		//"FOG", "1",
		NULL, NULL
	};

	mShaders["StandardVS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["OpaquePS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	mShaders["AlphaTestedPS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["BillboardVS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["BillboardGS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["BillboardPS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mBillboardLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}


void D3DFramework::BuildPSOs()
{
	// Direct3D���� ����� ������������ ���´� PSO��ü�� ���� �̸� �����ȴ�.
	// �� ��� ���� �ϳ��� ����ü�� �����ϴ� ������ Direct3D�� ��� ���°�
	// ȣȯ�Ǵ��� �̸� ������ �� ������ ����̹��� �ϵ���� ������ ���α׷�����
	// ���� ��� �ڵ带 �̸� ������ �� �ֱ� �����̴�.


	// PSO for opaque objects.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["StandardVS"]->GetBufferPointer()),
		mShaders["StandardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["OpaquePS"]->GetBufferPointer()),
		mShaders["OpaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = 1;
	opaquePsoDesc.SampleDesc.Quality = 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["Opaque"])));


	// PSO for Wireframe 
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["Wireframe"])));


	// PSO for Transparent
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;
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


	// PSO for AlphaTested
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["AlphaTestedPS"]->GetBufferPointer()),
		mShaders["AlphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["AlphaTested"])));


	// PSO for Billborad
	D3D12_GRAPHICS_PIPELINE_STATE_DESC billboardPsoDesc = opaquePsoDesc;
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
	billboardPsoDesc.BlendState.AlphaToCoverageEnable = m4xMsaaState;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&billboardPsoDesc, IID_PPV_ARGS(&mPSOs["Billborad"])));

}

void D3DFramework::BuildFrameResources()
{
	UINT objectCount = (UINT)std::distance(mAllObjects.begin(), mAllObjects.end());

	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources[i] = std::make_unique<FrameResource>(md3dDevice.Get(),
			1, objectCount, (UINT)mAssetManager->GetMaterials().size());
	}
}

void D3DFramework::BuildRenderItems()
{
	std::unique_ptr<GameObject> object;

	object = std::make_unique<GameObject>("Box");
	object->SetPosition(0.0f, 1.0f, 0.0f);
	object->SetScale(2.0f, 2.0f, 2.0f);
	object->SetMaterial(mAssetManager->FindMaterial("Wirefence"));
	object->SetMesh(mAssetManager->FindMesh("Box"));
	object->SetLayer(RenderLayer::AlphaTested);
	mAllObjects.push_front(std::move(object));

	object = std::make_unique<GameObject>("Floor");
	object->SetMaterial(mAssetManager->FindMaterial("Tile0"));
	object->SetMesh(mAssetManager->FindMesh("Grid"));
	object->SetLayer(RenderLayer::Opaque);
	mAllObjects.push_front(std::move(object));

	for (int i = 0; i < 5; ++i)
	{
		object = std::make_unique<GameObject>("ColumnLeft" + std::to_string(i));
		object->SetPosition(-5.0f, 1.5f, -10.0f + i * 5.0f);
		object->SetMaterial(mAssetManager->FindMaterial("Brick0"));
		object->SetMesh(mAssetManager->FindMesh("Cylinder"));
		object->SetLayer(RenderLayer::Opaque);
		mAllObjects.push_front(std::move(object));

		object = std::make_unique<GameObject>("ColumnRight" + std::to_string(i));
		object->SetPosition(5.0f, 1.5f, -10.0f + i * 5.0f);
		object->SetMaterial(mAssetManager->FindMaterial("Brick0"));
		object->SetMesh(mAssetManager->FindMesh("Cylinder"));
		object->SetLayer(RenderLayer::Opaque);
		mAllObjects.push_front(std::move(object));

		object = std::make_unique<GameObject>("SphereLeft" + std::to_string(i));
		object->SetPosition(-5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(mAssetManager->FindMaterial("Mirror0"));
		object->SetMesh(mAssetManager->FindMesh("Sphere"));
		object->SetLayer(RenderLayer::Transparent);
		mAllObjects.push_front(std::move(object));

		object = std::make_unique<GameObject>("SphereRight" + std::to_string(i));
		object->SetPosition(5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(mAssetManager->FindMaterial("Mirror0"));
		object->SetMesh(mAssetManager->FindMesh("Sphere"));
		object->SetLayer(RenderLayer::Transparent);
		mAllObjects.push_front(std::move(object));
	}
}

void D3DFramework::ClassifyObjectLayer()
{
	for (int i = 0; i < (int)RenderLayer::Count; ++i)
	{
		ClassifyObjectLayer(mLayerPair[i], i);
	}
}

void D3DFramework::ClassifyObjectLayer(std::pair<LayerIter, LayerIter>& layerPair, int i)
{
	// �� Layer�� Begin�� End�� �˾Ƴ���.
	//auto layerIter = std::equal_range(mAllObjects.begin(), mAllObjects.end(), i, ObjectLess());
	auto layerIterBegin = std::lower_bound(mAllObjects.begin(), mAllObjects.end(), i, ObjectLess());
	auto layerIterEnd = std::upper_bound(mAllObjects.begin(), mAllObjects.end(), i, ObjectLess());
	layerPair = std::make_pair(layerIterBegin, layerIterEnd);
}

void D3DFramework::RenderObjects(ID3D12GraphicsCommandList* cmdList, LayerIter iterBegin, LayerIter iterEnd)
{
	auto objectCB = mCurrentFrameResource->mObjectCB->GetResource();
	for (; iterBegin != iterEnd; ++iterBegin)
	{
		const GameObject* gameObject = static_cast<GameObject*>(iterBegin->get());

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + gameObject->GetObjectCBIndex() * objCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		if (gameObject->GetMesh() != nullptr)
			gameObject->GetMesh()->Render(cmdList);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> D3DFramework::GetStaticSamplers()
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

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp
	};
}
