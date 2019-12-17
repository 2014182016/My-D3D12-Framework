#include "pch.h"
#include "D3DFramework.h"
#include "Structures.h"
#include "FrameResource.h"
#include "UploadBuffer.h"
#include "GameTimer.h"
#include "Camera.h"
#include "D3DUtil.h"
#include "AssetManager.h"
#include "GameObject.h"
#include "Material.h"
#include "MeshGeometry.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Octree.h"
#include "D3DDebug.h"
#include "ShadowMap.h"
#include "MovingObject.h"
#include "MovingDirectionalLight.h"

using namespace DirectX;
using namespace std::literals;
using Microsoft::WRL::ComPtr;

D3DFramework::D3DFramework(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName)
	: D3DApp(hInstance, screenWidth, screenHeight, applicationName) 
{
	mCamera = std::make_unique<Camera>();
	mAssetManager = std::make_unique<AssetManager>();
	mMainPassCB = std::make_unique<PassConstants>();

	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
}

D3DFramework::~D3DFramework() { }


bool D3DFramework::Initialize()
{
	if (!__super::Initialize())
		return false;

	// �ʱ�ȭ ��ɵ��� �غ��ϱ� ���� ��� ��ϵ��� �缳���Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCamera->SetPosition(0.0f, 2.0f, -15.0f);

	mDebug = std::make_unique<D3DDebug>();

	mAssetManager->Initialize(md3dDevice.Get(), mCommandList.Get());
	mShadowMap = std::make_unique<ShadowMap>(md3dDevice.Get(), SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	BuildObjects();
	BuildLights();
	BuildFrameResources();

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildPSOs();

	// �ʱ�ȭ ��ɵ��� �����Ѵ�.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ʱ�ȭ�� ���� ������ ��ٸ���.
	FlushCommandQueue();

	for (const auto& obj : mAllObjects)
	{
		obj->BeginPlay();
		obj->Tick(0.0f); // WorldUpdate�� ���� Tick�Լ�
	}

	BoundingBox octreeAABB = BoundingBox(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(100.0f, 100.0f, 100.0f));
	mOctreeRoot = std::make_unique<Octree>(octreeAABB, mCollisionObjects);
	mOctreeRoot->BuildTree();

	return true;
}

void D3DFramework::OnDestroy()
{
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
	dsvHeapDesc.NumDescriptors = 1 + 1;
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

	DestroyObjects();

	mOctreeRoot->Update(deltaTime);
	for (auto& obj : mAllObjects)
	{
		obj->Tick(deltaTime);
	}

	UpdateLightBuffer(deltaTime);
	UpdateMaterialBuffer(deltaTime);
	UpdateMainPassCB(deltaTime);

	mWorldCamFrustum = mCamera->GetWorldCameraBounding();
}

void D3DFramework::Render()
{
	UINT objectCBIndex = 0;
	auto cmdListAlloc = mCurrentFrameResource->mCmdListAlloc;

	// ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� �缳���Ѵ�.
	// �缳���� GPU�� ���� ��ɸ�ϵ��� ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(cmdListAlloc->Reset());

	// ��� ����� ExcuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ�
	// ��� ����� �缳���� �� �ִ�. ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), nullptr));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvSrvUavDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// �� ��鿡 ���Ǵ� Pass ��� ���۸� ���´�.
	auto passCB = mCurrentFrameResource->mPassPool->GetBuffer()->GetResource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	// ������ ���۴� ���� �����ϰ� �׳� �ϳ��� ��Ʈ �����ڷ� ���� �� �ִ�.
	auto lightBuffer = mCurrentFrameResource->mLightBufferPool->GetBuffer()->GetResource();
	mCommandList->SetGraphicsRootShaderResourceView(2, lightBuffer->GetGPUVirtualAddress());

	auto matBuffer = mCurrentFrameResource->mMaterialBufferPool->GetBuffer()->GetResource();
	mCommandList->SetGraphicsRootShaderResourceView(3, matBuffer->GetGPUVirtualAddress());

	// �� ��鿡 ���Ǵ� ��� �ؽ�ó�� ���´�. ���̺��� ù �����ڸ� ������
	// ���̺� �� ���� �����ڰ� �ִ����� Root Signature�� �����Ǿ� �ִ�.
	mCommandList->SetGraphicsRootDescriptorTable(4, mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(mSkyCubeMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(6, skyTexDescriptor);

	// Shadow Pass
	// �׷��� �׸��� ���� ���� ���̴��� ���´�.
	CD3DX12_GPU_DESCRIPTOR_HANDLE shadowMapDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	shadowMapDescriptor.Offset(mShadowMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(5, shadowMapDescriptor);
	
	for (const auto& light : mLights)
	{
		if(light->IsUpdate())
			mShadowMap->RenderSceneToShadowMap(mCommandList.Get(), mPSOs["Shadow"].Get(), mCollisionObjects, objectCBIndex);
	}

	// Default Pass
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

	if (GetOptionEnabled(Option::Wireframe))
	{
		mCommandList->SetPipelineState(mPSOs["Wireframe"].Get());
		for (const auto& list : mGameObjects)
			RenderGameObjects(mCommandList.Get(), list, objectCBIndex, &mWorldCamFrustum);
	}
	else
	{
		mCommandList->SetPipelineState(mPSOs["Opaque"].Get());
		RenderGameObjects(mCommandList.Get(), mGameObjects[(int)RenderLayer::Opaque], objectCBIndex, &mWorldCamFrustum);

		mCommandList->SetPipelineState(mPSOs["AlphaTested"].Get());
		RenderGameObjects(mCommandList.Get(), mGameObjects[(int)RenderLayer::AlphaTested], objectCBIndex, &mWorldCamFrustum);

		mCommandList->SetPipelineState(mPSOs["Billborad"].Get());
		RenderGameObjects(mCommandList.Get(), mGameObjects[(int)RenderLayer::Billborad], objectCBIndex, &mWorldCamFrustum);

		mCommandList->SetPipelineState(mPSOs["Sky"].Get());
		RenderGameObjects(mCommandList.Get(), mGameObjects[(int)RenderLayer::Sky], objectCBIndex);

		mCommandList->SetPipelineState(mPSOs["Transparent"].Get());
		RenderGameObjects(mCommandList.Get(), mGameObjects[(int)RenderLayer::Transparent], objectCBIndex, &mWorldCamFrustum);
	}

#if defined(DEBUG) || defined(_DEBUG)
	if (GetOptionEnabled(Option::Debug_Collision) ||
		GetOptionEnabled(Option::Debug_Octree) ||
		GetOptionEnabled(Option::Debug_Light))
	{
		mDebug->Reset();
		mCommandList->SetPipelineState(mPSOs["CollisionDebug"].Get());

		if (GetOptionEnabled(Option::Debug_Collision))
		{
			DebugCollision(mCommandList.Get());
		}
		if (GetOptionEnabled(Option::Debug_Octree))
		{
			DebugOctree(mCommandList.Get());
		}
		if (GetOptionEnabled(Option::Debug_Light))
		{
			DebugLight(mCommandList.Get());
		}

		// ������ϱ� ���� �����͸� ������Ʈ�Ѵ�.
		auto debugPool = mCurrentFrameResource->mDebugPool.get();
		for (int i = 0; i < (int)DebugType::Count; ++i)
			mDebug->Update(mCommandList.Get(), debugPool, i);
		auto debugCBAddressStart = debugPool->GetBuffer()->GetResource()->GetGPUVirtualAddress();
		mCommandList->SetGraphicsRootShaderResourceView(7, debugCBAddressStart);

		auto instancePool = mCurrentFrameResource->mInstancePool.get();
		mDebug->RenderDebug(mCommandList.Get(), instancePool);
	}

	mCommandList->SetPipelineState(mPSOs["Line"].Get());
	mDebug->RenderLine(mCommandList.Get(), mGameTimer->GetDeltaTime());

	if (GetOptionEnabled(Option::Debug_Texture))
	{
		mCommandList->SetPipelineState(mPSOs["TextureDebug"].Get());
		RenderGameObjects(mCommandList.Get(), mGameObjects[(int)RenderLayer::TextureDebug], objectCBIndex);
	}
#endif

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

void D3DFramework::AddGameObject(std::shared_ptr<GameObject> object, RenderLayer renderLayer)
{
	mAllObjects.emplace_back(object);
	mGameObjects[(int)renderLayer].emplace_back(object);

	if (renderLayer == RenderLayer::Opaque || renderLayer == RenderLayer::Transparent || renderLayer == RenderLayer::AlphaTested)
		mCollisionObjects.emplace_back(object);

	++mObjectNum;

	if (mOctreeRoot)
		mOctreeRoot->Insert(object);

	// ������Ʈ�� ������ ������ ũ�⺸�� ũ�ٸ� Resize�Ѵ�.
	if (mCurrentFrameResource)
	{
		auto currObjectPool = mCurrentFrameResource->mObjectPool.get();
		if (currObjectPool->GetBufferCount() < mObjectNum)
			currObjectPool->Resize(mObjectNum * 2);
	}
}

void D3DFramework::AddLight(std::shared_ptr<class Light> object)
{
	mAllObjects.emplace_back(object);
	mLights.emplace_back(object);

	++mLightNum;
}

void D3DFramework::DestroyObjects()
{
	for (auto& list : mGameObjects)
		list.remove_if([](const std::shared_ptr<GameObject>& obj)->bool
			{ return obj->GetIsDestroyesd(); });

	mAllObjects.remove_if([](const std::shared_ptr<Object>& obj)->bool
	{ return obj->GetIsDestroyesd(); });

	mOctreeRoot->DestroyObjects();
}

void D3DFramework::UpdateLightBuffer(float deltaTime)
{
	auto currLightBuffer = mCurrentFrameResource->mLightBufferPool->GetBuffer();
	int i = 0;
	for (const auto& light : mLights)
	{
		if (light->IsUpdate())
		{
			LightData lightData;
			light->SetLightData(lightData, mSceneBounds);

			currLightBuffer->CopyData(i, lightData);

			light->DecreaseNumFrames();
		}
		++i;
	}
}

void D3DFramework::UpdateMaterialBuffer(float deltaTime)
{
	auto currMaterialBuffer = mCurrentFrameResource->mMaterialBufferPool->GetBuffer();
	for (const auto& e : mAssetManager->GetMaterials())
	{
		Material* mat = e.second.get();
		mat->Tick(deltaTime);
		if (mat->IsUpdate())
		{
			MaterialData matData;

			matData.mDiffuseAlbedo = mat->GetDiffuse();
			matData.mSpecular = mat->GetSpecular();
			matData.mRoughness = mat->GetRoughness();
			matData.mDiffuseMapIndex = mat->GetDiffuseIndex();
			matData.mNormalMapIndex = mat->GetNormalIndex();
			DirectX::XMStoreFloat4x4(&matData.mMatTransform, XMMatrixTranspose(mat->GetMaterialTransform()));

			currMaterialBuffer->CopyData(mat->GetMaterialIndex(), matData);

			mat->DecreaseNumFrames();
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

	XMStoreFloat4x4(&mMainPassCB->mView, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB->mInvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB->mProj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB->mInvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB->mViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB->mInvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB->mEyePosW = mCamera->GetPosition3f();
	mMainPassCB->mCurrentSkyCubeMapIndex = mCurrentSkyCubeMapIndex;
	mMainPassCB->mRenderTargetSize = XMFLOAT2((float)mScreenWidth, (float)mScreenHeight);
	mMainPassCB->mInvRenderTargetSize = XMFLOAT2(1.0f / mScreenWidth, 1.0f / mScreenHeight);
	mMainPassCB->mNearZ = 1.0f;
	mMainPassCB->mFarZ = 1000.0f;
	mMainPassCB->mTotalTime = mGameTimer->GetTotalTime();
	mMainPassCB->mDeltaTime = mGameTimer->GetDeltaTime();
	mMainPassCB->mAmbientLight = { 0.05f, 0.05f, 0.05f, 1.0f };
	mMainPassCB->mFogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	mMainPassCB->mFogStart = 5.0f;
	mMainPassCB->mFogRange = 10.0f;
	mMainPassCB->mFogEnabled = GetOptionEnabled(Option::Fog);

	auto passCB = mCurrentFrameResource->mPassPool->GetBuffer();
	passCB->CopyData(0, *mMainPassCB);
}

void D3DFramework::BuildRootSignature()
{
	// �Ϲ������� ���̴� ���α׷��� Ư�� �ڿ���(��� ����, �ؽ�ó, ǥ������� ��)��
	// �Էµȴٰ� ����Ѵ�. ��Ʈ ������ ���̴� ���α׷��� ����ϴ� �ڿ����� �����Ѵ�.

	std::array<CD3DX12_ROOT_PARAMETER, 9> slotRootParameter;

	CD3DX12_DESCRIPTOR_RANGE texTable[3];
	texTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)mAssetManager->GetTextures().size(), 0, 0);
	texTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)mLightNum, 0, 1);
	texTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)mAssetManager->GetCubeTextures().size(), 0, 2);

	// �����ս� TIP: ���� ���� ����ϴ� ���� �տ� ���´�.
	slotRootParameter[0].InitAsConstantBufferView(0); // Object ��� ����
	slotRootParameter[1].InitAsConstantBufferView(1); // Pass ��� ����
	slotRootParameter[2].InitAsShaderResourceView(0, 3); // Lights
	slotRootParameter[3].InitAsShaderResourceView(1, 3); // Materials
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable[0], D3D12_SHADER_VISIBILITY_PIXEL); // Textures
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable[1], D3D12_SHADER_VISIBILITY_PIXEL); // Shadow Maps
	slotRootParameter[6].InitAsDescriptorTable(1, &texTable[2], D3D12_SHADER_VISIBILITY_PIXEL); // CubeTextures
	slotRootParameter[7].InitAsShaderResourceView(2, 3); // Collision Debug
	slotRootParameter[8].InitAsConstantBufferView(2); // Collision Instance

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
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void D3DFramework::BuildDescriptorHeaps()
{
	UINT textureNum = (UINT)mAssetManager->GetTextures().size();
	UINT cubeTextureNum = (UINT)mAssetManager->GetCubeTextures().size();
	UINT shadowMapNum = mLightNum;

	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavDescriptorHeap = {};
	cbvSrvUavDescriptorHeap.NumDescriptors = textureNum + cubeTextureNum + shadowMapNum;
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
		ID3D12Resource* resource = mAssetManager->GetTextureResource(i);

		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource, &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	// �׸��� �� �����ڸ� �����ϱ� ���� �ʿ��� �������� �����Ѵ�.
	auto srvCpuStart = mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	mShadowMapHeapIndex = textureNum;
	mShadowMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, mDsvDescriptorSize));

	// ������ �ؽ�ó �ε������� ������������ ť�� ���� �����̴�.
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	mSkyCubeMapHeapIndex = textureNum + shadowMapNum;
	hDescriptor = mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	hDescriptor.Offset(mSkyCubeMapHeapIndex, mCbvSrvUavDescriptorSize);
	for (UINT i = 0; i < cubeTextureNum; ++i)
	{
		// texNames�� �ε����� Material�� ���� �ؽ�ó �ε����� �����ֱ� ���� ������� View�� �����Ѵ�.
		ID3D12Resource* resource = mAssetManager->GetCubeTextureResource(i);

		srvDesc.TextureCube.MipLevels = resource->GetDesc().MipLevels;
		srvDesc.Format = resource->GetDesc().Format;
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
		NULL, NULL
	};

	const D3D_SHADER_MACRO skyReflectionDefines[] =
	{
		"ALPHA_TEST", "1",
		//"SKY_REFLECTION", "1",
		NULL, NULL
	};

	mShaders["StandardVS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", skyReflectionDefines, "VS", "vs_5_1");
	mShaders["OpaquePS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", skyReflectionDefines, "PS", "ps_5_1");

	mShaders["WireframeVS"] = D3DUtil::CompileShader(L"Shaders\\Wireframe.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["WireframePS"] = D3DUtil::CompileShader(L"Shaders\\Wireframe.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["CollisionDebugVS"] = D3DUtil::CompileShader(L"Shaders\\CollisionDebug.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["CollisionDebugPS"] = D3DUtil::CompileShader(L"Shaders\\CollisionDebug.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["LineVS"] = D3DUtil::CompileShader(L"Shaders\\Line.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["LinePS"] = D3DUtil::CompileShader(L"Shaders\\Line.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SkyVS"] = D3DUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkyPS"] = D3DUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ShadowVS"] = D3DUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ShadowPS"] = D3DUtil::CompileShader(L"Shaders\\Shadow.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["TextureDebugVS"] = D3DUtil::CompileShader(L"Shaders\\TextureDebug.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["TextureDebugPS"] = D3DUtil::CompileShader(L"Shaders\\TextureDebug.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["AlphaTestedPS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["BillboardVS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["BillboardGS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["BillboardPS"] = D3DUtil::CompileShader(L"Shaders\\Billboard.hlsl", alphaTestDefines, "PS", "ps_5_1");

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

	mUILayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mCollisionDebugLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
	opaquePsoDesc.InputLayout = { mDefaultLayout.data(), (UINT)mDefaultLayout.size() };
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


	// PSO for Debug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC collisionDebugPsoDesc = opaqueWireframePsoDesc;
	collisionDebugPsoDesc.InputLayout = { mCollisionDebugLayout.data(), (UINT)mCollisionDebugLayout.size() };
	collisionDebugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["CollisionDebugVS"]->GetBufferPointer()),
		mShaders["CollisionDebugVS"]->GetBufferSize()
	};
	collisionDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["CollisionDebugPS"]->GetBufferPointer()),
		mShaders["CollisionDebugPS"]->GetBufferSize()
	};
	collisionDebugPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&collisionDebugPsoDesc, IID_PPV_ARGS(&mPSOs["CollisionDebug"])));


	// PSO for Line
	D3D12_GRAPHICS_PIPELINE_STATE_DESC linePsoDesc = collisionDebugPsoDesc;
	linePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["LineVS"]->GetBufferPointer()),
		mShaders["LineVS"]->GetBufferSize()
	};
	linePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["LinePS"]->GetBufferPointer()),
		mShaders["LinePS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&linePsoDesc, IID_PPV_ARGS(&mPSOs["Line"])));


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
	billboardPsoDesc.BlendState.AlphaToCoverageEnable = GetOptionEnabled(Option::Msaa);
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&billboardPsoDesc, IID_PPV_ARGS(&mPSOs["Billborad"])));


	// PSO for UI
	D3D12_GRAPHICS_PIPELINE_STATE_DESC uiPsoDesc = opaquePsoDesc;
	uiPsoDesc.InputLayout = { mUILayout.data(), (UINT)mUILayout.size() };
	uiPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["TextureDebugVS"]->GetBufferPointer()),
		mShaders["TextureDebugVS"]->GetBufferSize()
	};
	uiPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["TextureDebugPS"]->GetBufferPointer()),
		mShaders["TextureDebugPS"]->GetBufferSize()
	};
	uiPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	uiPsoDesc.DepthStencilState.DepthEnable = false;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&uiPsoDesc, IID_PPV_ARGS(&mPSOs["UI"])));

	
	// PSO for Sky
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;
	// �ɸ޶�� �ϴ� �� ���ο� �����Ƿ� �ĸ� ������ ����.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// ���� �Լ��� LESS�� �ƴ϶� LESS_EQUAL�� �����Ѵ�.
	// �̷��� ���� ������, ���� ���۸� 1�� ����� ��� z = 1(NDC)����
	// ����ȭ�� ���� ���� ���� ������ �����Ѵ�.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
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
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
	// Bias = (float)DepthBias * r + SlopeScaledDepthBias * MaxDepthSlope;
	// ���⼭ r�� ���� ���� ������ float32�� ��ȯ���� ��, ǥ�� ������ 0���� ū �ּڰ��̴�.
	// 24��Ʈ ���� ������ ��� r = 1 /2^24�̴�.
	// ex. DepthBias = 100000 -> ���� ���� ����ġ = 100000 / 2^24 = 0.006
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowVS"]->GetBufferPointer()),
		mShaders["ShadowVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ShadowPS"]->GetBufferPointer()),
		mShaders["ShadowPS"]->GetBufferSize()
	};
	// �׸��� �� �н����� ���� ����� ����.
	// �׸��� ���� ���� ���۸��� ����ϹǷ� ���� ����� ������� �������ν�
	// ������ ����ȭ�� ������ �� �ִ�.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["Shadow"])));


	// PSO for ShadowMapDebug
	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowMapDebugPsoDesc = uiPsoDesc;
	shadowMapDebugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["TextureDebugVS"]->GetBufferPointer()),
		mShaders["TextureDebugVS"]->GetBufferSize()
	};
	shadowMapDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["TextureDebugPS"]->GetBufferPointer()),
		mShaders["TextureDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowMapDebugPsoDesc, IID_PPV_ARGS(&mPSOs["TextureDebug"])));

}

void D3DFramework::BuildObjects()
{
	std::shared_ptr<GameObject> object;
	
	object = std::make_shared<GameObject>("Sky"s);
	object->SetScale(5000.0f, 5000.0f, 5000.0f);
	object->SetMaterial(mAssetManager->FindMaterial("Sky"s));
	object->SetMesh(mAssetManager->FindMesh("SkySphere"s));
	object->SetRenderLayer(RenderLayer::Sky);
	object->SetCollisionEnabled(false);
	AddGameObject(object, object->GetRenderLayer());

	object = std::make_shared<GameObject>("Floor0"s);
	object->SetScale(20.0f, 0.1f, 30.0f);
	object->SetMaterial(mAssetManager->FindMaterial("Tile0"s));
	object->SetMesh(mAssetManager->FindMesh("Cube_AABB"s));
	object->SetRenderLayer(RenderLayer::Opaque);
	AddGameObject(object, object->GetRenderLayer());

	object = std::make_shared<MovingObject>("MovingObject0");
	object->SetPosition(5.0f, 50.0f, 5.0f);
	object->SetMaterial(mAssetManager->FindMaterial("Default"s));
	object->SetMesh(mAssetManager->FindMesh("Cube_AABB"s));
	object->SetPhysics(true);
	object->SetMass(10.0f);
	AddGameObject(object, object->GetRenderLayer());

	object = std::make_shared<GameObject>("TextureDebug0"s);
	object->SetMaterial(mAssetManager->FindMaterial("Default"s));
	object->SetMesh(mAssetManager->FindMesh("Quad"s));
	object->SetRenderLayer(RenderLayer::TextureDebug);
	object->SetCollisionEnabled(false);
	AddGameObject(object, object->GetRenderLayer());

	for (int i = 0; i < 5; ++i)
	{
		object = std::make_shared<GameObject>("ColumnLeft"s + std::to_string(i));
		object->SetPosition(-5.0f, 1.5f, -10.0f + i * 5.0f);
		object->SetMaterial(mAssetManager->FindMaterial("Brick0"s));
		object->SetMesh(mAssetManager->FindMesh("Cylinder"s));
		object->SetRenderLayer(RenderLayer::Opaque);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("ColumnRight"s + std::to_string(i));
		object->SetPosition(5.0f, 1.5f, -10.0f + i * 5.0f);
		object->SetMaterial(mAssetManager->FindMaterial("Brick0"s));
		object->SetMesh(mAssetManager->FindMesh("Cylinder"s));
		object->SetRenderLayer(RenderLayer::Opaque);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("SphereLeft"s + std::to_string(i));
		object->SetPosition(-5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(mAssetManager->FindMaterial("Mirror0"s));
		object->SetMesh(mAssetManager->FindMesh("Sphere"s));
		object->SetRenderLayer(RenderLayer::Transparent);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("SphereRight"s + std::to_string(i));
		object->SetPosition(5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(mAssetManager->FindMaterial("Mirror0"s));
		object->SetMesh(mAssetManager->FindMesh("Sphere"s));
		object->SetRenderLayer(RenderLayer::Transparent);
		AddGameObject(object, object->GetRenderLayer());
	}
}

void D3DFramework::BuildLights()
{
	std::shared_ptr<DirectionalLight> dirLight;
	std::shared_ptr<PointLight> pointLight;
	std::shared_ptr<SpotLight> spotLight;

	dirLight = std::make_shared<DirectionalLight>("DirectionalLight0"s);
	dirLight->SetPosition(10.0f, 10.0f, -10.0f);
	dirLight->SetStrength(0.8f, 0.8f, 0.8f);
	dirLight->Rotate(45.0f, -45.0f, 0.0f);
	AddLight(dirLight);
}

void D3DFramework::BuildFrameResources()
{
	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources[i] = std::make_unique<FrameResource>(md3dDevice.Get(),
			1, mObjectNum * 2, mLightNum, (UINT)mAssetManager->GetMaterials().size());
	}
}

void D3DFramework::RenderGameObjects(ID3D12GraphicsCommandList* cmdList, const std::list<std::shared_ptr<GameObject>>& gameObjects,
	UINT& startObjectIndex, BoundingFrustum* frustum) const
{
	auto currObjectCB = mCurrentFrameResource->mObjectPool->GetBuffer();
	UINT& objectCBIndex = startObjectIndex;

	for (const auto& obj : gameObjects)
	{
		if (obj->GetMesh() == nullptr || !obj->GetIsVisible())
			continue;

		if (frustum != nullptr)
		{
			bool isInFrustum = obj->IsInFrustum(*frustum);
			// ī�޶� �������� ���� �ִٸ� �׸���.
			if (!isInFrustum)
				continue;
		}

		// ������Ʈ�� ��� ���۸� ������Ʈ�Ѵ�.
		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.mWorld, XMMatrixTranspose(obj->GetWorld()));
		objConstants.mMaterialIndex = obj->GetMaterial()->GetMaterialIndex();

		currObjectCB->CopyData(objectCBIndex, objConstants);
		auto objCBAddressStart = currObjectCB->GetResource()->GetGPUVirtualAddress();
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objCBAddressStart + objectCBIndex * objCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		obj->GetMesh()->Render(cmdList);

		++objectCBIndex;
	}
}

void D3DFramework::DebugCollision(ID3D12GraphicsCommandList* cmdList)
{
	auto debugPool = mCurrentFrameResource->mDebugPool.get();

	// GameObject�� ������ϱ� ���� �����͸� �����Ѵ�.
	for (const auto& list : mGameObjects)
	{
		for (const auto& obj : list)
		{
			auto boundingWorld = obj->GetBoundingWorld();
			if (!boundingWorld.has_value())
				continue;

			bool isInFrustum = obj->IsInFrustum(mWorldCamFrustum);
			if (isInFrustum)
			{
				XMFLOAT4X4 boundingWorld4x4f;
				XMStoreFloat4x4(&boundingWorld4x4f, XMMatrixTranspose(boundingWorld.value()));
				mDebug->AddDebugData(DebugData(boundingWorld4x4f), obj->GetCollisionType());
			}
		}
	}
}

void D3DFramework::DebugOctree(ID3D12GraphicsCommandList* cmdList)
{
	auto debugPool = mCurrentFrameResource->mDebugPool.get();

	// OctTree�� ������ϱ� ���� �����͸� �����Ѵ�.
	std::vector<DirectX::XMFLOAT4X4> worlds;
	mOctreeRoot->GetBoundingWorlds(worlds);
	for (const auto& world : worlds)
	{
		mDebug->AddDebugData(std::move(world), DebugType::Octree);
	}
}

void D3DFramework::DebugLight(ID3D12GraphicsCommandList* cmdList)
{
	auto debugPool = mCurrentFrameResource->mDebugPool.get();

	// Light�� ������ϱ� ���� �����͸� �����Ѵ�.
	for (const auto& light : mLights)
	{
		XMFLOAT3 pos = light->GetPosition();
		XMFLOAT3 rot = light->GetRotation();
		XMMATRIX translation = XMMatrixTranslation(pos.x, pos.y, pos.z);
		XMMATRIX rotation = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		XMMATRIX worldMat = XMMatrixMultiply(rotation, translation);
		XMFLOAT4X4 world;
		XMStoreFloat4x4(&world, XMMatrixTranspose(worldMat));

		mDebug->AddDebugData(std::move(world), DebugType::Light);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> D3DFramework::GetStaticSamplers()
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

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow,
	};
}

GameObject* D3DFramework::Picking(int screenX, int screenY, float distance) const
{
	XMFLOAT4X4 proj = mCamera->GetProj4x4f();

	// Picking Ray�� �� �������� ����Ѵ�.
	// Compute picking ray in view space.
	float vx = (2.0f * screenX / mScreenWidth - 1.0f) / proj(0, 0);
	float vy = (-2.0f * screenY / mScreenHeight + 1.0f) / proj(1, 1);

	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	XMMATRIX view = mCamera->GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	// Picking Ray�� ���� �������� ��ȯ�Ѵ�.
	rayOrigin = XMVector3TransformCoord(rayOrigin, invView);
	rayDir = XMVector3TransformNormal(rayDir, invView);
	rayDir = XMVector3Normalize(rayDir);

#if defined(DEBUG) || defined(_DEBUG)
	XMVECTOR rayEnd = rayOrigin + rayDir * distance;
	XMFLOAT3 xmf3RayOrigin, xmf3RayEnd;
	XMStoreFloat3(&xmf3RayOrigin, rayOrigin);
	XMStoreFloat3(&xmf3RayEnd, rayEnd);
	mDebug->DrawLine(xmf3RayOrigin, xmf3RayEnd);
#endif
	
	bool nearestHit = false;
	float nearestDist = FLT_MAX;
	GameObject* hitObj = nullptr;

	// Picking Ray�� �浹�� �˻��Ѵ�.
	for(const auto& list : mGameObjects)
		for (const auto& obj : list)
		{
			bool isHit = false;
			float hitDist = FLT_MAX;

			switch (obj->GetCollisionType())
			{
			case CollisionType::AABB: 
			{
				const BoundingBox& aabb = std::any_cast<BoundingBox>(obj->GetCollisionBounding());
				isHit = aabb.Intersects(rayOrigin, rayDir, hitDist);
				break;
			}
			case CollisionType::OBB: 
			{
				const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(obj->GetCollisionBounding());
				isHit = obb.Intersects(rayOrigin, rayDir, hitDist);
				break;
			}
			case CollisionType::Sphere: 
			{
				const BoundingSphere& sphere = std::any_cast<BoundingSphere>(obj->GetCollisionBounding());
				isHit = sphere.Intersects(rayOrigin, rayDir, hitDist);
				break;
			}
			}

			if (isHit)
			{
				// �浹�� Game Object�� �� ���� ����� ��ü�� ���õ� ��ü�̴�.
				if (nearestDist > hitDist)
				{
					nearestDist = hitDist;
					hitObj = obj.get();
					nearestHit = true;
				}
			}
		}

#if defined(DEBUG) || defined(_DEBUG)
	if (nearestHit)
	{
		std::cout << "Picking : " << hitObj->ToString() << std::endl;
	}
#endif

	return hitObj;
}
