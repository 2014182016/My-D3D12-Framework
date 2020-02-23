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
#include "Interfaces.h"
#include "Ssao.h"

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
	// 오직 하나의 Framework만이 존재한다.
	assert(instance == nullptr);
	instance = this;

	mMainPassCB = std::make_unique<PassConstants>();
	for (UINT i = 0; i < LIGHT_NUM; ++i)
		mShadowPassCB[i] = std::make_unique<PassConstants>();

	mCamera = std::make_unique<Camera>();

	BoundingBox octreeAABB = BoundingBox(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(100.0f, 100.0f, 100.0f));
	mOctreeRoot = std::make_unique<Octree>(octreeAABB);
	mOctreeRoot->BuildTree();
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

	mCamera->SetLens(60.0f, GetAspectRatio(), 1.0f, 1000.0f);

	for (auto& widget : mWidgets)
		widget->UpdateNumFrames();

#ifdef SSAO
	if(mSsao)
		mSsao->OnResize(md3dDevice.Get(), screenWidth, screenHeight);
#endif
}

void D3DFramework::InitFramework()
{
	Reset(mCommandList.Get(), mDirectCmdListAlloc.Get());

	mCamera->SetListener(mListener.Get());
	mCamera->SetPosition(0.0f, 2.0f, -15.0f);

	AssetManager::GetInstance()->Initialize(md3dDevice.Get(), mCommandList.Get(), md3dSound.Get());

#ifdef SSAO
	mSsao = std::make_unique<Ssao>(md3dDevice.Get(), mCommandList.Get(), mScreenWidth, mScreenHeight);
#endif

	CreateObjects();
	CreateLights();
	CreateWidgets();
	CreateParticles();
	CreateFrameResources();

	UINT textureNum = (UINT)AssetManager::GetInstance()->GetTextures().size();
	UINT cubeTextureNum = (UINT)AssetManager::GetInstance()->GetCubeTextures().size();
	UINT shadowMapNum = (UINT)LIGHT_NUM;

	CreateRootSignatures(textureNum, cubeTextureNum, shadowMapNum);
	CreateDescriptorHeaps(textureNum, cubeTextureNum, shadowMapNum);
	CreatePSOs();

	// 초기화 명령들을 실행한다.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 초기화가 끝날 때까지 기다린다.
	FlushCommandQueue();

	for (const auto& obj : mGameObjects)
		obj->BeginPlay();
}

void D3DFramework::CreateDescriptorHeaps(UINT textureNum, UINT cubeTextureNum, UINT shadowMapNum)
{
	D3DApp::CreateDescriptorHeaps(textureNum, cubeTextureNum, shadowMapNum);

	mSsaoMapHeapIndex = textureNum + cubeTextureNum + DEFERRED_BUFFER_COUNT + 1;
#ifdef SSAO
	mSsao->BuildDescriptors(md3dDevice.Get(),
		GetGpuSrv(textureNum + cubeTextureNum + 2),
		GetGpuSrv(textureNum + cubeTextureNum + DEFERRED_BUFFER_COUNT),
		GetCpuSrv(mSsaoMapHeapIndex),
		GetGpuSrv(mSsaoMapHeapIndex),
		GetRtv(SWAP_CHAIN_BUFFER_COUNT + DEFERRED_BUFFER_COUNT),
		mCbvSrvUavDescriptorSize,
		mRtvDescriptorSize);
#endif

	mShadowMapHeapIndex = mSsaoMapHeapIndex + 3;
	UINT i = 0;
	for (const auto& light : mLights)
	{
		light->GetShadowMap()->BuildDescriptors(md3dDevice.Get(),
			GetCpuSrv(mShadowMapHeapIndex + i),
			GetGpuSrv(mShadowMapHeapIndex + i),
			GetDsv(1));
		++i;
	}
}

void D3DFramework::SetCommonState(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->RSSetViewports(1, &mScreenViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvSrvUavDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootSignature(mRootSignatures["Common"].Get());

	// 구조적 버퍼는 힙을 생략하고 그냥 하나의 루트 서술자로 묶을 수 있다.
	cmdList->SetGraphicsRootShaderResourceView(RP_LIGHT, mCurrentFrameResource->GetLightVirtualAddress());
	cmdList->SetGraphicsRootShaderResourceView(RP_MATERIAL, mCurrentFrameResource->GetMaterialVirtualAddress());

	// 이 장면에 사용되는 모든 텍스처를 묶는다. 테이블의 첫 서술자만 묶으면
	// 테이블에 몇 개의 서술자가 있는지는 Root Signature에 설정되어 있다.
	cmdList->SetGraphicsRootDescriptorTable(RP_TEXTURE, GetGpuSrv(0));

	// 큐브 맵을 파이프라인에 바인딩한다.
	cmdList->SetGraphicsRootDescriptorTable(RP_CUBEMAP, GetGpuSrv(mSkyCubeMapHeapIndex));

	// G-Buffer에 사용될 렌더 타겟들을 파이프라인에 바인딩한다.
	cmdList->SetGraphicsRootDescriptorTable(RP_G_BUFFER, GetGpuSrv(mDeferredBufferHeapIndex));

	// 그림자 맵을 파이프라인에 바인딩한다.
	cmdList->SetGraphicsRootDescriptorTable(RP_SHADOWMAP, GetGpuSrv(mShadowMapHeapIndex));
}

void D3DFramework::Render()
{
	auto cmdListAlloc = mCurrentFrameResource->mCmdListAlloc;
	Reset(mCommandList.Get(), cmdListAlloc.Get());

	SetCommonState(mCommandList.Get());

	// Wireframe Pass ---------------------------------------------------------

	if (GetOptionEnabled(Option::Wireframe))
	{
		// 후면 버퍼를 그리기 위해 자원 상태를 전환한다.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// 후면 버퍼과 깊이 버퍼를 지운고, 파이프라인에 바인딩한다.
		mCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), (float*)&mBackBufferClearColor, 0, nullptr);
		mCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		mCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

		mCommandList->SetPipelineState(mPSOs["Forward"].Get());
		RenderActualObjects(&mWorldCamFrustum);

		// 와이어프레임으로 그려지는 것은 Deferred Rendering 및 라이팅이 필요없으므로 건너뛴다.
		goto RenderingPassSkip;
	}

	// Shadow Pass ---------------------------------------------------------

	{
		mCommandList->SetPipelineState(mPSOs["ShadowMap"].Get());
		UINT i = 0;
		for (const auto& light : mLights)
		{
			D3D12_GPU_VIRTUAL_ADDRESS shadowPassCBAddress = mCurrentFrameResource->GetPassVirtualAddress() + (1 + i++) * passCBByteSize;
			mCommandList->SetGraphicsRootConstantBufferView(RP_PASS, shadowPassCBAddress);

			light->RenderSceneToShadowMap(mCommandList.Get());
		}
	}

	// G-Buffer Pass ---------------------------------------------------------
	
	{
		// 이 장면에 사용되는 Pass 상수 버퍼를 묶는다.
		mCommandList->SetGraphicsRootConstantBufferView(RP_PASS, mCurrentFrameResource->GetPassVirtualAddress());

		// 각 G-Buffer의 버퍼들과 깊이 버퍼를 사용하기 위해 자원 상태를를 전환한다.
		CD3DX12_RESOURCE_BARRIER defferedBufferTransition[DEFERRED_BUFFER_COUNT];
		for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
			defferedBufferTransition[i] = CD3DX12_RESOURCE_BARRIER::Transition(mDeferredBuffer[i].Get(),
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mCommandList->ResourceBarrier(DEFERRED_BUFFER_COUNT, defferedBufferTransition);

		mCommandList->RSSetViewports(1, &mScreenViewport);
		mCommandList->RSSetScissorRects(1, &mScissorRect);

		// 각 G-Buffer의 버퍼들과 깊이 버퍼를 지운고, 파이프라인에 바인딩한다.
		for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
			mCommandList->ClearRenderTargetView(GetDefferedBufferView(i), (float*)&mDeferredBufferClearColors[i], 0, nullptr);
		mCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		mCommandList->OMSetRenderTargets(DEFERRED_BUFFER_COUNT, &GetDefferedBufferView(0), true, &GetDepthStencilView());

		mCommandList->SetPipelineState(mPSOs["Opaque"].Get());
		RenderObjects(mRenderableObjects[(int)RenderLayer::Opaque], mCurrentFrameResource->GetObjectVirtualAddress(), RP_OBJECT, objCBByteSize, true, &mWorldCamFrustum);

		mCommandList->SetPipelineState(mPSOs["AlphaTested"].Get());
		RenderObjects(mRenderableObjects[(int)RenderLayer::AlphaTested], mCurrentFrameResource->GetObjectVirtualAddress(), RP_OBJECT, objCBByteSize, true, &mWorldCamFrustum);

		mCommandList->SetPipelineState(mPSOs["Billborad"].Get());
		RenderObjects(mRenderableObjects[(int)RenderLayer::Billborad], mCurrentFrameResource->GetObjectVirtualAddress(), RP_OBJECT, objCBByteSize, true, &mWorldCamFrustum);

		// 각 G-Buffer의 버퍼들과 깊이 버퍼를 읽기 위해 자원 상태를를 전환한다.
		for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
			defferedBufferTransition[i] = CD3DX12_RESOURCE_BARRIER::Transition(mDeferredBuffer[i].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
		mCommandList->ResourceBarrier(DEFERRED_BUFFER_COUNT, defferedBufferTransition);
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

#ifdef SSAO
	// Ssao Pass--------------------------------------------------------------
	{
		mSsao->ComputeSsao(mCommandList.Get(), mPSOs["Ssao"].Get(), mCurrentFrameResource->GetSsaoVirtualAddress());
		mSsao->BlurAmbientMap(mCommandList.Get(), mPSOs["SsaoBlur"].Get(), mCurrentFrameResource->GetSsaoVirtualAddress(), 3);
	}

	SetCommonState(mCommandList.Get());
#endif

	// Lighting Pass ---------------------------------------------------------

	{
		// 후면 버퍼를 그리기 위해 자원 상태를 전환한다.
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		// 후면 버퍼과 깊이 버퍼를 지운고, 파이프라인에 바인딩한다.
		mCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), (float*)&mBackBufferClearColor, 0, nullptr);
		mCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

		mCommandList->SetPipelineState(mPSOs["LightingPass"].Get());
		auto iter = mWidgets.begin();
		RenderObject((*iter).get(), mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize, false);
	}

	// Forward Pass ---------------------------------------------------------

	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		mCommandList->SetPipelineState(mPSOs["Particle"].Get());
		RenderObjects(mRenderableObjects[(int)RenderLayer::Particle], mCurrentFrameResource->GetParticleVirtualAddress(), RP_PARTICLE, particleCBByteSize, true, &mWorldCamFrustum);

		mCommandList->SetPipelineState(mPSOs["Sky"].Get());
		RenderObjects(mRenderableObjects[(int)RenderLayer::Sky], mCurrentFrameResource->GetObjectVirtualAddress(), RP_OBJECT, objCBByteSize);

		mCommandList->SetPipelineState(mPSOs["Transparent"].Get());
		RenderObjects(mRenderableObjects[(int)RenderLayer::Transparent], mCurrentFrameResource->GetObjectVirtualAddress(), RP_OBJECT, objCBByteSize, true, &mWorldCamFrustum);
	}

RenderingPassSkip:

	// Widget Pass ---------------------------------------------------------

	{
		mCommandList->SetPipelineState(mPSOs["Widget"].Get());
		RenderObjects(mRenderableObjects[(int)RenderLayer::Widget], mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize);

#if defined(DEBUG) || defined(_DEBUG)
		if (GetOptionEnabled(Option::Debug_GBuffer))
		{
			// 위젯의 1~6번째까지는 G-Buffer를 그리는 위젯이다.
			auto iter = mWidgets.begin();
			++iter;

			mCommandList->SetPipelineState(mPSOs["DiffuseMapDebug"].Get());
			RenderObject((*iter).get(), mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["SpecularMapDebug"].Get());
			RenderObject((*iter).get(), mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["RoughnessMapDebug"].Get());
			RenderObject((*iter).get(), mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["NormalMapDebug"].Get());
			RenderObject((*iter).get(), mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize, false);
			++iter;

			mCommandList->SetPipelineState(mPSOs["DepthMapDebug"].Get());
			RenderObject((*iter).get(), mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize, false);
			++iter;

#ifdef SSAO
			mCommandList->SetPipelineState(mPSOs["SsaoMapDebug"].Get());
			RenderObject((*iter).get(), mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize, false);
			++iter;
#endif

			mCommandList->SetPipelineState(mPSOs["ShadowMapDebug"].Get());
			RenderObject((*iter).get(), mCurrentFrameResource->GetWidgetVirtualAddress(), RP_WIDGET, widgetCBByteSize, false);
		}
#endif
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 전면 버퍼와 후면 버퍼를 바꾼다.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

	// 현재 울타리 지점까지의 명령들을 표시하도록 울타리 값을 전진시킨다.
	mCurrentFrameResource->mFence = ++mCurrentFence;

	// 새 울타리 지점을 설정하는 명령(Signal)을 명령 대기열에 추가한다.
	// 새 울타리 지점은 GPU가 이 Signal() 명령까지의 모든 명령을 처리하기
	// 전까지는 설정되지 않는다.
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void D3DFramework::Tick(float deltaTime)
{
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % NUM_FRAME_RESOURCES;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	// GPU가 현재 FrameResource까지 명령을 끝맞쳤는지 확인한다.
	// 아니라면 Fence Point에 도달할 떄까지 기다린다.
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

#ifdef SSAO
	UpdateSsaoBuffer(deltaTime);
#endif
}


void D3DFramework::AddGameObject(std::shared_ptr<GameObject> object, RenderLayer renderLayer)
{
	object->SetRenderLayer(renderLayer);
	mGameObjects.push_back(object);
	mRenderableObjects[(int)renderLayer].push_back(object);

	if (renderLayer == RenderLayer::Opaque || renderLayer == RenderLayer::Transparent || renderLayer == RenderLayer::AlphaTested)
	{
		object->WorldUpdate();

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
	mGameObjects.remove_if([](const std::shared_ptr<Object>& obj)->bool
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

	for (auto& obj : mGameObjects)
	{
		obj->Tick(deltaTime);

		if (objectIndex != obj->GetCBIndex())
		{
			obj->SetCBIndex(objectIndex);
			obj->UpdateNumFrames();
		}

		if (obj->IsUpdate())
		{
			// 오브젝트의 상수 버퍼를 업데이트한다.
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
	// NDC 공간 [-1, 1]^2을 텍스처 공간 [0, 1]^2으로 변환하는 행렬
	static XMMATRIX toTextureTransform(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	auto currLightBuffer = mCurrentFrameResource->mLightBufferPool->GetBuffer();
	auto passCB = mCurrentFrameResource->mPassPool->GetBuffer();
	UINT i = 0;
	for (const auto& light : mLights)
	{
		light->Tick(deltaTime);
		if (light->IsUpdate())
		{
			LightData lightData;
			light->SetLightData(lightData);

			XMMATRIX view = light->GetView();
			XMMATRIX proj = light->GetProj();

			XMMATRIX viewProj = XMMatrixMultiply(view, proj);
			XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
			XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
			XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

			XMStoreFloat4x4(&mShadowPassCB[i]->mView, XMMatrixTranspose(view));
			XMStoreFloat4x4(&mShadowPassCB[i]->mInvView, XMMatrixTranspose(invView));
			XMStoreFloat4x4(&mShadowPassCB[i]->mProj, XMMatrixTranspose(proj));
			XMStoreFloat4x4(&mShadowPassCB[i]->mInvProj, XMMatrixTranspose(invProj));
			XMStoreFloat4x4(&mShadowPassCB[i]->mViewProj, XMMatrixTranspose(viewProj));
			XMStoreFloat4x4(&mShadowPassCB[i]->mInvViewProj, XMMatrixTranspose(invViewProj));
			mShadowPassCB[i]->mEyePosW = light->GetPosition();
			mShadowPassCB[i]->mRenderTargetSize = XMFLOAT2((float)SHADOW_MAP_SIZE, (float)SHADOW_MAP_SIZE);
			mShadowPassCB[i]->mInvRenderTargetSize = XMFLOAT2(1.0f / SHADOW_MAP_SIZE, 1.0f / SHADOW_MAP_SIZE);
			mShadowPassCB[i]->mNearZ = 0.5f;
			mShadowPassCB[i]->mFarZ = light->GetFalloffEnd();

			currLightBuffer->CopyData(i, lightData);
			passCB->CopyData(1 + i, *mShadowPassCB[i]);

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
	static const XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

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
	XMStoreFloat4x4(&mMainPassCB->mProjTex, XMMatrixTranspose(proj * T));
	XMStoreFloat4x4(&mMainPassCB->mViewProjTex, XMMatrixTranspose(viewProj * T));
	mMainPassCB->mEyePosW = mCamera->GetPosition3f();
	mMainPassCB->mCurrentSkyCubeMapIndex = mCurrentSkyCubeMapIndex;
	mMainPassCB->mRenderTargetSize = XMFLOAT2((float)mScreenWidth, (float)mScreenHeight);
	mMainPassCB->mInvRenderTargetSize = XMFLOAT2(1.0f / mScreenWidth, 1.0f / mScreenHeight);
	mMainPassCB->mNearZ = mCamera->GetNearZ();
	mMainPassCB->mFarZ = mCamera->GetFarZ();
	mMainPassCB->mTotalTime = mGameTimer->GetTotalTime();
	mMainPassCB->mDeltaTime = mGameTimer->GetDeltaTime();
	mMainPassCB->mAmbientLight = { 0.4f, 0.4f, 0.6f, 1.0f };
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
	// [0, 1]범위로 설정된 위젯의 정점 위치를 동차 좌표계의 [-1, 1]범위로 변환한다.
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

void D3DFramework::UpdateSsaoBuffer(float deltaTime)
{
	static const XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	SsaoConstants ssaoCB;

	mSsao->GetOffsetVectors(ssaoCB.mOffsetVectors);
	auto blurWeights = mSsao->CalcGaussWeights(2.5f);
	ssaoCB.mBlurWeights[0] = XMFLOAT4(&blurWeights[0]);
	ssaoCB.mBlurWeights[1] = XMFLOAT4(&blurWeights[4]);
	ssaoCB.mBlurWeights[2] = XMFLOAT4(&blurWeights[8]);

	ssaoCB.mOcclusionRadius = 0.5f;
	ssaoCB.mOcclusionFadeStart = 0.2f;
	ssaoCB.mOcclusionFadeEnd = 1.0f;
	ssaoCB.mSurfaceEpsilon = 0.05f;

	ssaoCB.mSsaoContrast = 4.0f;

	auto currSsaoCB = mCurrentFrameResource->mSsaoPool->GetBuffer();
	currSsaoCB->CopyData(0, ssaoCB);
}

void D3DFramework::UpdateObjectBufferPool()
{
	UINT allObjectCount = (UINT)mGameObjects.size();

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
	object->SetCollisionEnabled(false);
	AddGameObject(object, RenderLayer::Sky);

	object = std::make_shared<GameObject>("Floor0"s);
	object->SetScale(20.0f, 0.1f, 30.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tile0"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Cube_AABB"s));
	object->SetCollisionEnabled(true);
	AddGameObject(object, RenderLayer::Opaque);


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
		object->SetCollisionEnabled(true);
		AddGameObject(object, RenderLayer::Opaque);

		object = std::make_shared<GameObject>("SphereLeft"s + std::to_string(i));
		object->SetPosition(-5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Mirror0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Sphere"s));
		object->SetCollisionEnabled(true);
		AddGameObject(object, RenderLayer::Transparent);

		object = std::make_shared<GameObject>("SphereRight"s + std::to_string(i));
		object->SetPosition(5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Mirror0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Sphere"s));
		object->SetCollisionEnabled(true);
		AddGameObject(object, RenderLayer::Transparent);
	}
}

void D3DFramework::CreateLights()
{
	std::shared_ptr<Light> light;

	light = std::make_shared<DirectionalLight>("DirectionalLight"s, md3dDevice.Get());
	light->SetPosition(10.0f, 10.0f, -10.0f);
	light->SetStrength(0.8f, 0.8f, 0.8f);
	light->Rotate(45.0f, -45.0f, 0.0f);
	light->SetFalloffStart(10.0f);
	light->SetFalloffEnd(50.0f);
	light->SetShadowMapSize(50.0f, 50.0f);
	light->SetSpotAngle(45.0f);
	mLights.push_back(std::move(light));

	if ((UINT)mLights.size() != LIGHT_NUM)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Max Light Num Difference Error" << std::endl;
#endif
	}
}

void D3DFramework::CreateWidgets()
{
	std::shared_ptr<Widget> widget;

	widget = std::make_shared<Widget>("RenderTarget"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetSize(mScreenWidth, mScreenHeight);
	widget->SetAnchor(0.0f, 0.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mRenderableObjects[(int)RenderLayer::Widget].push_back(widget);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("DiffuseMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(0, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	widget->SetRenderLayer(RenderLayer::Widget);
	mRenderableObjects[(int)RenderLayer::Widget].push_back(widget);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("SpecularMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(160, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mRenderableObjects[(int)RenderLayer::Widget].push_back(widget);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("RoughnessMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(320, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mRenderableObjects[(int)RenderLayer::Widget].push_back(widget);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("NormalMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(480, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mRenderableObjects[(int)RenderLayer::Widget].push_back(widget);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("DetphMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(640, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mRenderableObjects[(int)RenderLayer::Widget].push_back(widget);
	mWidgets.push_back(std::move(widget));
	
	widget = std::make_shared<Widget>("SsaoMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(800, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mRenderableObjects[(int)RenderLayer::Widget].push_back(widget);
	mWidgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("ShadowMapDebug"s, md3dDevice.Get(), mCommandList.Get());
	widget->SetPosition(960, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widget->SetVisible(false);
	mRenderableObjects[(int)RenderLayer::Widget].push_back(widget);
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
	particle->SetRenderLayer(RenderLayer::Particle);
	mRenderableObjects[(int)RenderLayer::Particle].push_back(particle);
	mParticles.push_back(std::move(particle));
	*/
}

void D3DFramework::CreateFrameResources()
{
	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources[i] = std::make_unique<FrameResource>(md3dDevice.Get(),
			1 + LIGHT_NUM, (UINT)mGameObjects.size() * 2, LIGHT_NUM,
			(UINT)AssetManager::GetInstance()->GetMaterials().size(), (UINT)mWidgets.size(), (UINT)mParticles.size());

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
	UINT rootParameterIndex, UINT strideCBByteSize, bool visibleCheck, BoundingFrustum* frustum) const
{
	if (obj->GetMesh() == nullptr)
		return;

	if (visibleCheck)
	{
		if (!obj->GetIsVisible())
			return;

		if (frustum != nullptr)
		{
			bool isInFrustum = obj->IsInFrustum(frustum);
			if (!isInFrustum)
				return;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = addressStarat + obj->GetCBIndex() * strideCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, cbAddress);

	obj->Render(mCommandList.Get());
}

void D3DFramework::RenderObjects(const std::list<std::shared_ptr<Renderable>>& list, D3D12_GPU_VIRTUAL_ADDRESS addressStarat,
	UINT rootParameterIndex, UINT strideCBByteSize, bool visibleCheck, BoundingFrustum* frustum) const
{
	for (const auto& obj : list)
		RenderObject(obj.get(), addressStarat, rootParameterIndex, strideCBByteSize, visibleCheck, frustum);
}

void D3DFramework::RenderActualObjects(DirectX::BoundingFrustum* frustum)
{
	auto currObjectCB = mCurrentFrameResource->mObjectPool->GetBuffer();
	D3D12_GPU_VIRTUAL_ADDRESS addressStarat = currObjectCB->GetResource()->GetGPUVirtualAddress();

	for (const auto& obj : mRenderableObjects[(int)RenderLayer::Opaque])
		RenderObject(obj.get(), addressStarat, RP_OBJECT, objCBByteSize, true, frustum);

	for (const auto& obj : mRenderableObjects[(int)RenderLayer::AlphaTested])
		RenderObject(obj.get(), addressStarat, RP_OBJECT, objCBByteSize, true, frustum);

	for (const auto& obj : mRenderableObjects[(int)RenderLayer::Billborad])
		RenderObject(obj.get(), addressStarat, RP_OBJECT, objCBByteSize, true, frustum);

	for (const auto& obj : mRenderableObjects[(int)RenderLayer::Transparent])
		RenderObject(obj.get(), addressStarat, RP_OBJECT, objCBByteSize, true, frustum);
}

GameObject* D3DFramework::Picking(int screenX, int screenY, float distance, bool isMeshCollision) const
{
	XMFLOAT4X4 proj = mCamera->GetProj4x4f();

	// Picking Ray를 뷰 공간으로 계산한다.
	float vx = (2.0f * screenX / mScreenWidth - 1.0f) / proj(0, 0);
	float vy = (-2.0f * screenY / mScreenHeight + 1.0f) / proj(1, 1);

	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	XMMATRIX view = mCamera->GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	// Picking Ray를 월드 공간으로 변환한다.
	rayOrigin = XMVector3TransformCoord(rayOrigin, invView);
	rayDir = XMVector3TransformNormal(rayDir, invView);
	rayDir = XMVector3Normalize(rayDir);

	bool nearestHit = false;
	float nearestDist = FLT_MAX;
	GameObject* hitObj = nullptr;

	// Picking Ray로 충돌을 검사한다.
	for (const auto& obj : mGameObjects)
	{
		bool isHit = false;
		float hitDist = FLT_MAX;

		CollisionType collisionType = obj->GetCollisionType();
		if (isMeshCollision)
		{
			if (obj->GetMesh())
				collisionType = obj->GetMesh()->GetCollisionType();
		}

		switch (collisionType)
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
			// 충돌된 Game Object들 중 가장 가까운 객체가 선택된 객체이다.
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
