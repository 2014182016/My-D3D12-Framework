#include "pch.h"
#include "D3DFramework.h"
#include "AssetManager.h"
#include "D3DUtil.h"
#include "Structures.h"
#include "FrameResource.h"
#include "UploadBuffer.h"
#include "GameTimer.h"
#include "Camera.h"
#include "InputManager.h"
#include "Octree.h"
#include "ShadowMap.h"

#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Widget.h"
#include "Particle.h"
#include "Billboard.h"

using namespace DirectX;
using namespace std::literals;
using Microsoft::WRL::ComPtr;

D3DFramework::D3DFramework(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName)
	: D3DApp(hInstance, screenWidth, screenHeight, applicationName) 
{
	// ���� �ϳ��� Framework���� �����Ѵ�.
	assert(instance == nullptr);
	instance = this;

	mMainPassCB = std::make_unique<PassConstants>();
	mCamera = std::make_unique<Camera>();
	mShadowMap = std::make_unique<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	BoundingBox octreeAABB = BoundingBox(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(100.0f, 100.0f, 100.0f));
	mOctreeRoot = std::make_unique<Octree>(octreeAABB);
	mOctreeRoot->BuildTree();

	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
}

D3DFramework::~D3DFramework() { }


bool D3DFramework::Initialize()
{
	if (!__super::Initialize())
		return false;

	InitFramework();

	return true;
}

void D3DFramework::OnDestroy()
{
	__super::OnDestroy();
}

void D3DFramework::OnResize(int screenWidth, int screenHeight)
{
	__super::OnResize(screenWidth, screenHeight);

	mCamera->SetLens(0.25f*DirectX::XM_PI, GetAspectRatio(), 1.0f, 1000.0f);

	for (auto& widget : mWidgets)
		widget->UpdateNumFrames();
}

void D3DFramework::InitFramework()
{
	// �ʱ�ȭ ��ɵ��� �غ��ϱ� ���� ��� ��ϵ��� �缳���Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCamera->SetListener(mListener.Get());
	mCamera->SetPosition(0.0f, 2.0f, -15.0f);

	mShadowMap->Initialize(md3dDevice.Get());
	AssetManager::GetInstance()->Initialize(md3dDevice.Get(), mCommandList.Get(), md3dSound.Get());
	SetBackgroundColor(XMFLOAT4(Colors::LightSkyBlue));

	CreateObjects();
	CreateLights();
	CreateWidgets();
	CreateParticles();
	CreateFrameResources();

	UINT textureNum = (UINT)AssetManager::GetInstance()->GetTextures().size();
	UINT cubeTextureNum = (UINT)AssetManager::GetInstance()->GetCubeTextures().size();
	UINT shadowMapNum = (UINT)mLights.size();

	CreateRootSignature(textureNum, cubeTextureNum, shadowMapNum);
	CreateDescriptorHeaps(textureNum, cubeTextureNum, shadowMapNum);
	CreateShadowMapResource(textureNum, cubeTextureNum);
	CreatePSOs();

	// �ʱ�ȭ ��ɵ��� �����Ѵ�.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ʱ�ȭ�� ���� ������ ��ٸ���.
	FlushCommandQueue();

	for (const auto& list : mGameObjects)
	{
		for (const auto& obj : list)
			obj->BeginPlay();
	}
}

void D3DFramework::CreateShadowMapResource(UINT textureNum, UINT cubeTextureNum)
{
	// �׸��� �� �����ڸ� �����ϱ� ���� �ʿ��� �������� �����Ѵ�.
	auto srvCpuStart = mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	mShadowMapHeapIndex = textureNum + cubeTextureNum + DEFERRED_BUFFER_COUNT + 1;
	mShadowMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, mDsvDescriptorSize));
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

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvSrvUavDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// �� ��鿡 ���Ǵ� Pass ��� ���۸� ���´�.
	auto passCB = mCurrentFrameResource->mPassPool->GetBuffer()->GetResource();
	mCommandList->SetGraphicsRootConstantBufferView(RP_PASS, passCB->GetGPUVirtualAddress());

	// ������ ���۴� ���� �����ϰ� �׳� �ϳ��� ��Ʈ �����ڷ� ���� �� �ִ�.
	auto lightBuffer = mCurrentFrameResource->mLightBufferPool->GetBuffer()->GetResource();
	mCommandList->SetGraphicsRootShaderResourceView(RP_LIGHT, lightBuffer->GetGPUVirtualAddress());

	auto matBuffer = mCurrentFrameResource->mMaterialBufferPool->GetBuffer()->GetResource();
	mCommandList->SetGraphicsRootShaderResourceView(RP_MATERIAL, matBuffer->GetGPUVirtualAddress());

	// �� ��鿡 ���Ǵ� ��� �ؽ�ó�� ���´�. ���̺��� ù �����ڸ� ������
	// ���̺� �� ���� �����ڰ� �ִ����� Root Signature�� �����Ǿ� �ִ�.
	mCommandList->SetGraphicsRootDescriptorTable(RP_TEXTURE, mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// ť�� ���� ���������ο� ���ε��Ѵ�.
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(mSkyCubeMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(RP_CUBEMAP, skyTexDescriptor);

	// G-Buffer�� ���� ���� Ÿ�ٵ��� ���������ο� ���ε��Ѵ�.
	CD3DX12_GPU_DESCRIPTOR_HANDLE defferedBufferDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	defferedBufferDescriptor.Offset(mDeferredBufferHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(RP_G_BUFFER, defferedBufferDescriptor);

	// �׸��� ���� ���������ο� ���ε��Ѵ�.
	CD3DX12_GPU_DESCRIPTOR_HANDLE shadowMapDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	shadowMapDescriptor.Offset(mShadowMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(RP_SHADOWMAP, shadowMapDescriptor);

	if (GetOptionEnabled(Option::Wireframe))
	{
		// �ĸ� ���۸� �׸��� ���� �ڿ� ���¸� ��ȯ�Ѵ�.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		// �ĸ� ���۰� ���� ���۸� �����, ���������ο� ���ε��Ѵ�.
		mCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), (float*)&mBackBufferClearColor, 0, nullptr);
		mCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		mCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

		mCommandList->SetPipelineState(mPSOs["Forward"].Get());
		RenderGameObjects(mActualObjects, &mWorldCamFrustum);

		// ���̾����������� �׷����� ���� Deferred Rendering �� �������� �ʿ�����Ƿ� �ǳʶڴ�.
		goto RenderingPassSkip;
	}

	// Shadow Pass ---------------------------------------------------------

	{
		mCommandList->SetPipelineState(mPSOs["ShadowMap"].Get());
		mShadowMap->RenderSceneToShadowMap(mCommandList.Get(), mActualObjects);
	}

	// G-Buffer Pass ---------------------------------------------------------
	
	{
		// �� G-Buffer�� ���۵�� ���� ���۸� ����ϱ� ���� �ڿ� ���¸��� ��ȯ�Ѵ�.
		CD3DX12_RESOURCE_BARRIER defferedBufferTransition[DEFERRED_BUFFER_COUNT];
		for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
			defferedBufferTransition[i] = CD3DX12_RESOURCE_BARRIER::Transition(mDeferredBuffer[i].Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mCommandList->ResourceBarrier(DEFERRED_BUFFER_COUNT, defferedBufferTransition);
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		// �� G-Buffer�� ���۵�� ���� ���۸� �����, ���������ο� ���ε��Ѵ�.
		for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
			mCommandList->ClearRenderTargetView(GetDefferedBufferView(i), (float*)&mBackBufferClearColor, 0, nullptr);
		mCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		mCommandList->OMSetRenderTargets(DEFERRED_BUFFER_COUNT, &GetDefferedBufferView(0), true, &GetDepthStencilView());

		mCommandList->SetPipelineState(mPSOs["Opaque"].Get());
		RenderGameObjects(mGameObjects[(int)RenderLayer::Opaque], &mWorldCamFrustum);

		mCommandList->SetPipelineState(mPSOs["AlphaTested"].Get());
		RenderGameObjects(mGameObjects[(int)RenderLayer::AlphaTested], &mWorldCamFrustum);

		mCommandList->SetPipelineState(mPSOs["Billborad"].Get());
		RenderGameObjects(mGameObjects[(int)RenderLayer::Billborad], &mWorldCamFrustum);

		// �� G-Buffer�� ���۵�� ���� ���۸� �б� ���� �ڿ� ���¸��� ��ȯ�Ѵ�.
		for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
			defferedBufferTransition[i] = CD3DX12_RESOURCE_BARRIER::Transition(mDeferredBuffer[i].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
		mCommandList->ResourceBarrier(DEFERRED_BUFFER_COUNT, defferedBufferTransition);
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

	// Lighting Pass ---------------------------------------------------------

	{
		// �ĸ� ���۸� �׸��� ���� �ڿ� ���¸� ��ȯ�Ѵ�.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		// �ĸ� ���۰� ���� ���۸� �����, ���������ο� ���ε��Ѵ�.
		mCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), (float*)&mBackBufferClearColor, 0, nullptr);
		mCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

		mCommandList->SetPipelineState(mPSOs["LightingPass"].Get());
		auto iter = mWidgets.begin();
		std::advance(iter, 6);
		auto currWidgetCB = mCurrentFrameResource->mWidgetPool->GetBuffer();
		RenderObject((*iter).get(), currWidgetCB->GetResource()->GetGPUVirtualAddress(), RP_WIDGET, (*iter)->GetCBIndex(), widgetCBByteSize, false);
	}

	// Forward Pass ---------------------------------------------------------

	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		mCommandList->SetPipelineState(mPSOs["Particle"].Get());
		RenderParticles();

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ));

		mCommandList->SetPipelineState(mPSOs["Sky"].Get());
		RenderGameObjects(mGameObjects[(int)RenderLayer::Sky]);

		mCommandList->SetPipelineState(mPSOs["Transparent"].Get());
		RenderGameObjects(mGameObjects[(int)RenderLayer::Transparent], &mWorldCamFrustum);
	}

RenderingPassSkip:

	// Widget Pass ---------------------------------------------------------

	{
		mCommandList->SetPipelineState(mPSOs["Widget"].Get());
		RenderWidgets();

#if defined(DEBUG) || defined(_DEBUG)
		if (GetOptionEnabled(Option::Debug_GBuffer))
		{
			// ������ 0~4��°������ G-Buffer�� �׸��� �����̴�.
			auto iter = mWidgets.begin();
			auto currWidgetCB = mCurrentFrameResource->mWidgetPool->GetBuffer();

			mCommandList->SetPipelineState(mPSOs["DiffuseMapDebug"].Get());
			RenderObject((*iter).get(), currWidgetCB->GetResource()->GetGPUVirtualAddress(), RP_WIDGET, (*iter)->GetCBIndex(), widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["SpecularMapDebug"].Get());
			RenderObject((*iter).get(), currWidgetCB->GetResource()->GetGPUVirtualAddress(), RP_WIDGET, (*iter)->GetCBIndex(), widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["RoughnessMapDebug"].Get());
			RenderObject((*iter).get(), currWidgetCB->GetResource()->GetGPUVirtualAddress(), RP_WIDGET, (*iter)->GetCBIndex(), widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["NormalMapDebug"].Get());
			RenderObject((*iter).get(), currWidgetCB->GetResource()->GetGPUVirtualAddress(), RP_WIDGET, (*iter)->GetCBIndex(), widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["DepthMapDebug"].Get());
			RenderObject((*iter).get(), currWidgetCB->GetResource()->GetGPUVirtualAddress(), RP_WIDGET, (*iter)->GetCBIndex(), widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["ShadowMapDebug"].Get());
			RenderObject((*iter).get(), currWidgetCB->GetResource()->GetGPUVirtualAddress(), RP_WIDGET, (*iter)->GetCBIndex(), widgetCBByteSize, false);
		}
#endif
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

	mWorldCamFrustum = mCamera->GetWorldCameraBounding();

	DestroyGameObjects();
	DestroyParticles();

	mOctreeRoot->Update(deltaTime);
	UpdateObjectBuffer(deltaTime);
	UpdateLightBuffer(deltaTime);
	UpdateMaterialBuffer(deltaTime);
	UpdateMainPassBuffer(deltaTime);
	UpdateWidgetBuffer(deltaTime);
	UpdateParticleBuffer(deltaTime);
}


void D3DFramework::AddGameObject(std::shared_ptr<GameObject> object, RenderLayer renderLayer)
{
	mGameObjects[(int)renderLayer].push_back(object);

	if (renderLayer == RenderLayer::Opaque || renderLayer == RenderLayer::Transparent || renderLayer == RenderLayer::AlphaTested)
	{
		object->WorldUpdate();
		mActualObjects.push_back(object);

		if (mOctreeRoot)
		{
			if(object->GetCollisionType() != CollisionType::None)
				mOctreeRoot->Insert(object);
		}
	}

	UpdateObjectBufferPool();
}

void D3DFramework::DestroyGameObjects()
{
	for (auto& list : mGameObjects)
	{
		list.remove_if([](const std::shared_ptr<Object>& obj)->bool
		{ return obj->GetIsDestroyesd(); });
	}

	mActualObjects.remove_if([](const std::shared_ptr<Object>& obj)->bool
	{ return obj->GetIsDestroyesd(); });

	mOctreeRoot->DestroyObjects();
}

void D3DFramework::DestroyParticles()
{
	for (auto iter = mParticles.begin(); iter != mParticles.end();)
	{
		auto& particle = *iter;
		if (particle->GetIsDestroyesd())
		{
			auto& particleVBs = mCurrentFrameResource->mParticleVBs;
			auto vbIter = particleVBs.begin() + particle->GetCBIndex();
			particleVBs.erase(vbIter);
			iter = mParticles.erase(iter);
			continue;
		}
		else
		{
			++iter;
		}
	}
}

void D3DFramework::UpdateObjectBuffer(float deltaTime)
{
	auto currObjectCB = mCurrentFrameResource->mObjectPool->GetBuffer();
	UINT objectIndex = 0;

	for(auto& list : mGameObjects)
		for (auto& obj : list)
		{
			obj->Tick(deltaTime);

			if (objectIndex != obj->GetCBIndex())
			{
				obj->SetCBIndex(objectIndex);
				obj->UpdateNumFrames();
			}

			if (obj->IsUpdate())
			{
				// ������Ʈ�� ��� ���۸� ������Ʈ�Ѵ�.
				ObjectConstants objConstants;
				XMStoreFloat4x4(&objConstants.mWorld, XMMatrixTranspose(obj->GetWorld()));
				objConstants.mMaterialIndex = obj->GetMaterial()->GetMaterialIndex();

				currObjectCB->CopyData(objectIndex, objConstants);
			}
			++objectIndex;
		}
}

void D3DFramework::UpdateLightBuffer(float deltaTime)
{
	auto currLightBuffer = mCurrentFrameResource->mLightBufferPool->GetBuffer();
	UINT i = 0;
	for (const auto& light : mLights)
	{
		light->Tick(deltaTime);
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
	for (const auto& e : AssetManager::GetInstance()->GetMaterials())
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

void D3DFramework::UpdateMainPassBuffer(float deltaTime)
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
	mMainPassCB->mNearZ = mCamera->GetNearZ();
	mMainPassCB->mFarZ = mCamera->GetFarZ();
	mMainPassCB->mTotalTime = mGameTimer->GetTotalTime();
	mMainPassCB->mDeltaTime = mGameTimer->GetDeltaTime();
	mMainPassCB->mAmbientLight = { 0.05f, 0.05f, 0.05f, 1.0f };
	mMainPassCB->mFogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	mMainPassCB->mFogStart = 0.0f;
	mMainPassCB->mFogRange = 100.0f;
	mMainPassCB->mFogDensity = 0.1f;
	mMainPassCB->mFogEnabled = GetOptionEnabled(Option::Fog);
	mMainPassCB->mFogType = (std::uint32_t)FogType::Exponential;

	auto passCB = mCurrentFrameResource->mPassPool->GetBuffer();
	passCB->CopyData(0, *mMainPassCB);
}

void D3DFramework::UpdateWidgetBuffer(float deltaTime)
{
	// [0, 1]������ ������ ������ ���� ��ġ�� ���� ��ǥ���� [-1, 1]������ ��ȯ�Ѵ�.
	static std::function<float(float)> transformHomogenous = [](float x) { return x * 2.0f - 1.0f; };

	auto currWidgetPool = mCurrentFrameResource->mWidgetPool->GetBuffer();
	auto& currWidgetVBs = mCurrentFrameResource->mWidgetVBs;
	UINT widgetIndex = 0;

	for (const auto& widget : mWidgets)
	{
		widget->Tick(deltaTime);
		if (widgetIndex != widget->GetCBIndex())
		{
			widget->SetCBIndex(widgetIndex);
			widget->UpdateNumFrames();
		}

		if (widget->IsUpdate())
		{
			WidgetConstants widgetConstants;

			if(widget->GetMaterial())
				widgetConstants.mMaterialIndex = widget->GetMaterial()->GetMaterialIndex();
			currWidgetPool->CopyData(widgetIndex, widgetConstants);

			float posX = (float)widget->GetPosX();
			float posY = (float)widget->GetPosY();
			float width = (float)widget->GetWidth();
			float height = (float)widget->GetHeight();
			float anchorX = widget->GetAnchorX();
			float anchorY = widget->GetAnchorY();

			float minX = anchorX + (posX / mScreenWidth);
			float minY = anchorY + (posY / mScreenHeight);
			float maxX = anchorX + ((posX + width) / mScreenWidth);
			float maxY = anchorY + ((posY + height) / mScreenHeight);

			minX = transformHomogenous(minX);
			minY = transformHomogenous(minY);
			maxX = transformHomogenous(maxX);
			maxY = transformHomogenous(maxY);

			WidgetVertex vertices[4];
			vertices[0] = WidgetVertex(XMFLOAT3(minX, minY, 0.0f), XMFLOAT2(0.0f, 1.0f));
			vertices[1] = WidgetVertex(XMFLOAT3(minX, maxY, 0.0f), XMFLOAT2(0.0f, 0.0f));
			vertices[2] = WidgetVertex(XMFLOAT3(maxX, minY, 0.0f), XMFLOAT2(1.0f, 1.0f));
			vertices[3] = WidgetVertex(XMFLOAT3(maxX, maxY, 0.0f), XMFLOAT2(1.0f, 0.0f));

			for (int i = 0; i < 4; ++i)
				currWidgetVBs[widgetIndex]->CopyData(i, vertices[i]);
			widget->GetMesh()->SetDynamicVertexBuffer(currWidgetVBs[widgetIndex]->GetResource(),
				4, (UINT)sizeof(WidgetVertex));

			widget->DecreaseNumFrames();
		}
		++widgetIndex;
	}
}

void D3DFramework::UpdateParticleBuffer(float deltaTime)
{
	auto currParticlePool = mCurrentFrameResource->mParticlePool->GetBuffer();
	auto& currParticleVBs = mCurrentFrameResource->mParticleVBs;
	UINT particleIndex = 0;

	for (const auto& particle : mParticles)
	{
		particle->Tick(deltaTime);
		if (particleIndex != particle->GetCBIndex())
		{
			particle->SetCBIndex(particleIndex);
			particle->UpdateNumFrames();
		}

		if (particle->IsUpdate())
		{
			ParticleConstants particleConstants;

			particleConstants.mMaterialIndex = particle->GetMaterial()->GetMaterialIndex();
			particleConstants.mFacingCamera = true;

			currParticlePool->CopyData(particleIndex, particleConstants);

			auto& particleDatas = particle->GetParticleDatas();
			UINT vertexIndex = 0;
			for (auto& paritlceData : particleDatas)
			{
				ParticleVertex v;
				v.mPos = paritlceData->mPosition;
				v.mColor = paritlceData->mColor;
				v.mNormal = paritlceData->mNormal;
				v.mSize = paritlceData->mSize;
				currParticleVBs[particleIndex]->CopyData(vertexIndex++, v);
			}
			particle->GetMesh()->SetDynamicVertexBuffer(currParticleVBs[particleIndex]->GetResource(),
				vertexIndex, (UINT)sizeof(ParticleVertex));

			particle->DecreaseNumFrames();
		}
		++particleIndex;
	}
}

void D3DFramework::UpdateObjectBufferPool()
{
	UINT allObjectCount = 0;

	for (const auto& list : mGameObjects)
		allObjectCount += (UINT)list.size();

	if (mCurrentFrameResource)
	{
		auto currObjectPool = mCurrentFrameResource->mObjectPool.get();
		if (currObjectPool->GetBufferCount() < allObjectCount)
			currObjectPool->Resize(allObjectCount * 2);
	}
}

void D3DFramework::CreateObjects()
{
	std::shared_ptr<GameObject> object;
	
	object = std::make_shared<GameObject>("Sky"s);
	object->SetScale(5000.0f, 5000.0f, 5000.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Sky"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("SkySphere"s));
	object->SetRenderLayer(RenderLayer::Sky);
	object->SetCollisionEnabled(false);
	AddGameObject(object, object->GetRenderLayer());

	object = std::make_shared<GameObject>("Floor0"s);
	object->SetScale(20.0f, 0.1f, 30.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tile0"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Cube_AABB"s));
	object->SetRenderLayer(RenderLayer::Opaque);
	object->SetCollisionEnabled(true);
	AddGameObject(object, object->GetRenderLayer());

	for (int i = 0; i < 5; ++i)
	{
		object = std::make_shared<GameObject>("ColumnLeft"s + std::to_string(i));
		object->SetPosition(-5.0f, 1.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Brick0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Cylinder"s));
		object->SetRenderLayer(RenderLayer::Opaque);
		object->SetCollisionEnabled(true);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("ColumnRight"s + std::to_string(i));
		object->SetPosition(5.0f, 1.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Brick0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Cylinder"s));
		object->SetRenderLayer(RenderLayer::Opaque);
		object->SetCollisionEnabled(true);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("SphereLeft"s + std::to_string(i));
		object->SetPosition(-5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Mirror0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Sphere"s));
		object->SetRenderLayer(RenderLayer::Transparent);
		object->SetCollisionEnabled(true);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("SphereRight"s + std::to_string(i));
		object->SetPosition(5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Mirror0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Sphere"s));
		object->SetRenderLayer(RenderLayer::Transparent);
		object->SetCollisionEnabled(true);
		AddGameObject(object, object->GetRenderLayer());
	}
}

void D3DFramework::CreateLights()
{
	std::shared_ptr<DirectionalLight> dirLight;
	std::shared_ptr<PointLight> pointLight;
	std::shared_ptr<SpotLight> spotLight;

	dirLight = std::make_shared<DirectionalLight>("DirectionalLight0"s);
	dirLight->SetPosition(10.0f, 10.0f, -10.0f);
	dirLight->SetStrength(0.8f, 0.8f, 0.8f);
	dirLight->Rotate(45.0f, -45.0f, 0.0f);
	mLights.push_back(std::move(dirLight));
}

void D3DFramework::CreateWidgets()
{
	std::shared_ptr<Widget> widget;

	widget = std::make_shared<Widget>("DiffuseMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(0, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("SpecularMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(160, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("RoughnessMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(320, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("NormalMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(480, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("DetphMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(640, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("ShadowMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(960, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("RenderTarget"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetSize(mScreenWidth, mScreenHeight);
	widget->SetAnchor(0.0f, 0.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mWidgets.push_back(std::move(widget));
}

void D3DFramework::CreateParticles()
{
	std::shared_ptr<Particle> particle;

	/*
	particle = std::make_shared<Particle>("Particle0"s, md3dDevice.Get(), mCommandList.Get(), 500);
	particle->SetPosition(0.0f, 5.0f, 0.0f);
	particle->SetSpawnDistanceRange(XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f));
	particle->SetVelocityRange(XMFLOAT3(-10.0f, -10.0f, -10.0f), XMFLOAT3(10.0f, 10.0f, 10.0f));
	particle->SetColorRange(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	particle->SetSizeRange(XMFLOAT2(2.0f, 2.0f));
	particle->SetSpawnTimeRange(0.1f, 0.2f);
	particle->SetLifeTimeRange(1.5f, 3.0f);
	particle->SetLifeTime(10.0f);
	particle->SetBurstNum(5);
	particle->SetEnabledGravity(true);
	particle->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree0"s));
	mParticles.push_back(std::move(particle));
	*/
}

void D3DFramework::CreateFrameResources()
{
	UINT gameObjectNum = 0;
	for (const auto& list : mGameObjects)
		gameObjectNum += (UINT)list.size();

	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources[i] = std::make_unique<FrameResource>(md3dDevice.Get(),
			1, gameObjectNum * 2, (UINT)mLights.size(), (UINT)AssetManager::GetInstance()->GetMaterials().size(),
			(UINT)mWidgets.size(), (UINT)mParticles.size());

		mFrameResources[i]->mWidgetVBs.reserve((UINT)mWidgets.size());
		for (const auto& widget : mWidgets)
		{
			std::unique_ptr<UploadBuffer<WidgetVertex>> vb = std::make_unique<UploadBuffer<WidgetVertex>>(md3dDevice.Get(), 4, false);
			mFrameResources[i]->mWidgetVBs.push_back(std::move(vb));
		}

		mFrameResources[i]->mParticleVBs.reserve((UINT)mParticles.size());
		for (const auto& particle : mParticles)
		{
			std::unique_ptr<UploadBuffer<ParticleVertex>> vb = std::make_unique<UploadBuffer<ParticleVertex>>(md3dDevice.Get(), particle->GetMaxParticleNum(), false);
			mFrameResources[i]->mParticleVBs.push_back(std::move(vb));
		}
	}
}

void D3DFramework::RenderObject(Renderable* obj, D3D12_GPU_VIRTUAL_ADDRESS addressStarat, 
	UINT rootParameterIndex, UINT cbIndex, UINT strideCBByteSize, bool visibleCheck) const
{
	if (visibleCheck)
	{
		if (obj->GetMesh() == nullptr || !obj->GetIsVisible())
			return;
	}

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = addressStarat + cbIndex * strideCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, cbAddress);

	obj->Render(mCommandList.Get());
}

void D3DFramework::RenderGameObjects(const std::list<std::shared_ptr<GameObject>>& gameObjects, const BoundingFrustum* frustum) const
{
	auto currObjectCB = mCurrentFrameResource->mObjectPool->GetBuffer();

	for (const auto& obj : gameObjects)
	{
		if (frustum != nullptr)
		{
			bool isInFrustum = obj->IsInFrustum(*frustum);
			// ī�޶� �������� ���� �ִٸ� �׸���.
			if (!isInFrustum)
				continue;
		}

		RenderObject(obj.get(), currObjectCB->GetResource()->GetGPUVirtualAddress(), RP_OBJECT, obj->GetCBIndex(), objCBByteSize);
	}
}

void D3DFramework::RenderWidgets() const
{
	auto currWidgetCB = mCurrentFrameResource->mWidgetPool->GetBuffer();

	for (const auto& widget : mWidgets)
	{
		RenderObject(widget.get(), currWidgetCB->GetResource()->GetGPUVirtualAddress(), RP_WIDGET, widget->GetCBIndex(), widgetCBByteSize);
	}
}

void D3DFramework::RenderParticles() const
{
	auto currParticleCB = mCurrentFrameResource->mParticlePool->GetBuffer();

	for (const auto& particle : mParticles)
	{
		RenderObject(particle.get(), currParticleCB->GetResource()->GetGPUVirtualAddress(), RP_PARTICLE, particle->GetCBIndex(), particleCBByteSize);
	}
}

GameObject* D3DFramework::Picking(int screenX, int screenY, float distance) const
{
	XMFLOAT4X4 proj = mCamera->GetProj4x4f();

	// Picking Ray�� �� �������� ����Ѵ�.
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

	bool nearestHit = false;
	float nearestDist = FLT_MAX;
	GameObject* hitObj = nullptr;

	// Picking Ray�� �浹�� �˻��Ѵ�.
	for (const auto& obj : mActualObjects)
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
