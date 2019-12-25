#include "pch.h"
#include "D3DFramework.h"
#include "AssetManager.h"
#include "D3DDebug.h"
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
#include "MeshGeometry.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Widget.h"
#include "MovingObject.h"
#include "MovingDirectionalLight.h"

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
}

void D3DFramework::InitFramework()
{
	// �ʱ�ȭ ��ɵ��� �غ��ϱ� ���� ��� ��ϵ��� �缳���Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCamera->SetListener(mListener.Get());
	mCamera->SetPosition(0.0f, 2.0f, -15.0f);

	mShadowMap->Initialize(md3dDevice.Get());
	D3DDebug::GetInstance()->Initialize(md3dDevice.Get());
	AssetManager::GetInstance()->Initialize(md3dDevice.Get(), mCommandList.Get(), md3dSound.Get());

	CreateObjects();
	CreateLights();
	CreateUIs();
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

	for (const auto& obj : mAllObjects)
	{
		obj->BeginPlay();
	}
}

void D3DFramework::CreateShadowMapResource(UINT textureNum, UINT cubeTextureNum)
{
	// �׸��� �� �����ڸ� �����ϱ� ���� �ʿ��� �������� �����Ѵ�.
	auto srvCpuStart = mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	mShadowMapHeapIndex = textureNum + cubeTextureNum;
	mShadowMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, mDsvDescriptorSize));
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

	DestroyObjects();

	mOctreeRoot->Update(deltaTime);
	for (auto& obj : mAllObjects)
	{
		obj->Tick(deltaTime);
	}

	UpdateLightBuffer(deltaTime);
	UpdateMaterialBuffer(deltaTime);
	UpdateMainPassCB(deltaTime);
	UpdateWidgetCB(deltaTime);

#if defined(DEBUG) || defined(_DEBUG)
	if (GetOptionEnabled(Option::Debug_Collision) ||
		GetOptionEnabled(Option::Debug_Octree) ||
		GetOptionEnabled(Option::Debug_Light))
	{
		D3DDebug::GetInstance()->Reset();

		if (GetOptionEnabled(Option::Debug_Collision))
		{
			UpdateDebugCollision(mCommandList.Get());
		}
		if (GetOptionEnabled(Option::Debug_Octree))
		{
			UpdateDebugOctree(mCommandList.Get());
		}
		if (GetOptionEnabled(Option::Debug_Light))
		{
			UpdateDebugLight(mCommandList.Get());
		}

		UpdateDebugBufferPool();
	}
#endif
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

	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(mSkyCubeMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(RP_CUBEMAP, skyTexDescriptor);

	// Shadow Pass
	// �׷��� �׸��� ���� ���� ���̴��� ���´�.
	CD3DX12_GPU_DESCRIPTOR_HANDLE shadowMapDescriptor(mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	shadowMapDescriptor.Offset(mShadowMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(RP_SHADOWMAP, shadowMapDescriptor);
	
	UINT objectCBIndex = 0;
	for (const auto& light : mLights)
	{
		if(light->IsUpdate())
			mShadowMap->RenderSceneToShadowMap(mCommandList.Get(), mPSOs["Shadow"].Get(), mActualObjects, objectCBIndex);
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
		mCommandList->SetPipelineState(mPSOs["CollisionDebug"].Get());
		RenderCollisionDebug(mCommandList.Get());
	}

	mCommandList->SetPipelineState(mPSOs["Line"].Get());
	D3DDebug::GetInstance()->RenderLine(mCommandList.Get(), mGameTimer->GetDeltaTime());
#endif

	mCommandList->SetPipelineState(mPSOs["Widget"].Get());
	RenderWidget(mCommandList.Get());

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
	{
		object->WorldUpdate();
		mActualObjects.emplace_back(object);

		if (mOctreeRoot)
			mOctreeRoot->Insert(object);
	}

	UpdateObjectBufferPool();
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

void D3DFramework::UpdateWidgetCB(float deltaTime)
{
	auto currWidgetPool = mCurrentFrameResource->mWidgetPool->GetBuffer();
	UINT i = 0;
	for (const auto& widget : mWidgets)
	{
		widget->Tick(deltaTime);
		if (widget->IsUpdate())
		{
			WidgetConstants widgetConstants;

			widgetConstants.mMaterialIndex = widget->GetMaterial()->GetMaterialIndex();
			widgetConstants.mPosX = widget->GetPosX();
			widgetConstants.mPosY = widget->GetPosY();
			widgetConstants.mWidth = widget->GetWidth();
			widgetConstants.mHeight = widget->GetHeight();
			widgetConstants.mAnchorX = widget->GetAnchorX();
			widgetConstants.mAnchorY = widget->GetAnchotY();

			currWidgetPool->CopyData(i, widgetConstants);

			widget->DecreaseNumFrames();
		}
		++i;
	}
}

void D3DFramework::UpdateDebugCollision(ID3D12GraphicsCommandList* cmdList)
{
	std::vector<DebugConstants>& aabbConstatns = D3DDebug::GetInstance()->GetDebugConstants((int)DebugType::AABB);
	std::vector<DebugConstants>& obbConstatns = D3DDebug::GetInstance()->GetDebugConstants((int)DebugType::OBB);
	std::vector<DebugConstants>& sphereConstatns = D3DDebug::GetInstance()->GetDebugConstants((int)DebugType::Sphere);

	// GameObject�� ������ϱ� ���� �����͸� �����Ѵ�.
	for (const auto& list : mGameObjects)
	{
		for (const auto& obj : list)
		{
			auto boundingWorld = obj->GetBoundingWorld();
			if (!boundingWorld.has_value())
				continue;

			bool isInFrustum = obj->IsInFrustum(mWorldCamFrustum);
			if (!isInFrustum)
				continue;

			XMFLOAT4X4 boundingWorld4x4f;
			XMStoreFloat4x4(&boundingWorld4x4f, XMMatrixTranspose(boundingWorld.value()));

			switch (obj->GetCollisionType())
			{
			case CollisionType::AABB:
				aabbConstatns.emplace_back(DebugConstants(std::move(boundingWorld4x4f), (DirectX::XMFLOAT4)DirectX::Colors::Red));
				break;
			case CollisionType::OBB:
				obbConstatns.emplace_back(DebugConstants(std::move(boundingWorld4x4f), (DirectX::XMFLOAT4)DirectX::Colors::Red));
				break;
			case CollisionType::Sphere:
				sphereConstatns.emplace_back(DebugConstants(std::move(boundingWorld4x4f), (DirectX::XMFLOAT4)DirectX::Colors::Red));
				break;
			}
		}
	}
}

void D3DFramework::UpdateDebugOctree(ID3D12GraphicsCommandList* cmdList)
{
	std::vector<DebugConstants>& octreeConstatns = D3DDebug::GetInstance()->GetDebugConstants((int)DebugType::Octree);

	// OctTree�� ������ϱ� ���� �����͸� �����Ѵ�.
	std::vector<DirectX::XMFLOAT4X4> worlds;
	mOctreeRoot->GetBoundingWorlds(worlds);
	for (const auto& world : worlds)
	{
		octreeConstatns.emplace_back(DebugConstants(std::move(world), (DirectX::XMFLOAT4)DirectX::Colors::Green));
	}
}

void D3DFramework::UpdateDebugLight(ID3D12GraphicsCommandList* cmdList)
{
	std::vector<DebugConstants>& lightConstatns = D3DDebug::GetInstance()->GetDebugConstants((int)DebugType::Light);

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

		lightConstatns.emplace_back(DebugConstants(std::move(world), (DirectX::XMFLOAT4)DirectX::Colors::Yellow));
	}
}

void D3DFramework::UpdateDebugBufferPool()
{
	auto debugPool = mCurrentFrameResource->mDebugPool.get();
	UINT debugDataCount = D3DDebug::GetInstance()->GetDebugDataCount();
	if (debugDataCount > debugPool->GetBufferCount())
		debugPool->Resize(debugDataCount * 2);
}

void D3DFramework::UpdateObjectBufferPool()
{
	UINT gameObjectNum = 0;
	for (const auto& list : mGameObjects)
		gameObjectNum += (UINT)list.size();

	if (mCurrentFrameResource)
	{
		auto currObjectPool = mCurrentFrameResource->mObjectPool.get();
		if (currObjectPool->GetBufferCount() < gameObjectNum)
			currObjectPool->Resize(gameObjectNum * 2);
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
	AddGameObject(object, object->GetRenderLayer());

	object = std::make_shared<MovingObject>("MovingObject0");
	object->SetPosition(5.0f, 50.0f, 5.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Cube_AABB"s));
	object->SetPhysics(true);
	object->SetMass(10.0f);
	AddGameObject(object, object->GetRenderLayer());

	for (int i = 0; i < 5; ++i)
	{
		object = std::make_shared<GameObject>("ColumnLeft"s + std::to_string(i));
		object->SetPosition(-5.0f, 1.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Brick0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Cylinder"s));
		object->SetRenderLayer(RenderLayer::Opaque);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("ColumnRight"s + std::to_string(i));
		object->SetPosition(5.0f, 1.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Brick0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Cylinder"s));
		object->SetRenderLayer(RenderLayer::Opaque);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("SphereLeft"s + std::to_string(i));
		object->SetPosition(-5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Mirror0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Sphere"s));
		object->SetRenderLayer(RenderLayer::Transparent);
		AddGameObject(object, object->GetRenderLayer());

		object = std::make_shared<GameObject>("SphereRight"s + std::to_string(i));
		object->SetPosition(5.0f, 3.5f, -10.0f + i * 5.0f);
		object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Mirror0"s));
		object->SetMesh(AssetManager::GetInstance()->FindMesh("Sphere"s));
		object->SetRenderLayer(RenderLayer::Transparent);
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
	mLights.emplace_back(std::move(dirLight));
}

void D3DFramework::CreateUIs()
{
	std::shared_ptr<Widget> widget;

	widget = std::make_shared<Widget>("Widget0"s);
	widget->SetSize(400, 300);
	widget->SetAnchor(0.5f, 0.5f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tile0"s));
	//mWidgets.emplace_back(std::move(widget));
}

void D3DFramework::CreateFrameResources()
{
	UINT gameObjectNum = 0;
	for (const auto& list : mGameObjects)
		gameObjectNum += (UINT)list.size();

	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources[i] = std::make_unique<FrameResource>(md3dDevice.Get(),
			1, gameObjectNum * 2, (UINT)mLights.size(), (UINT)AssetManager::GetInstance()->GetMaterials().size(), (UINT)mWidgets.size());
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
		cmdList->SetGraphicsRootConstantBufferView(RP_OBJECT, objCBAddress);

		obj->GetMesh()->Render(cmdList);

		++objectCBIndex;
	}
}

void D3DFramework::RenderCollisionDebug(ID3D12GraphicsCommandList* cmdList)
{
	// ������ϱ� ���� �����͸� ������Ʈ�Ѵ�.
	auto debugPool = mCurrentFrameResource->mDebugPool.get();

	UINT debugCBIndex = 0;
	for (int i = 0; i < (int)DebugType::Count; ++i)
	{
		const std::vector<DebugConstants>& currConstants = D3DDebug::GetInstance()->GetDebugConstants(i);
		if (currConstants.size() == 0)
			continue;

		for (const auto& constant : currConstants)
		{
			// ����� �����͸� ���ۿ� ������Ʈ�Ѵ�.
			debugPool->GetBuffer()->CopyData(debugCBIndex, constant);

			auto debugCBAddressStart = debugPool->GetBuffer()->GetResource()->GetGPUVirtualAddress();
			D3D12_GPU_VIRTUAL_ADDRESS debugCBAddress = debugCBAddressStart + debugCBIndex * debugCBByteSize;
			mCommandList->SetGraphicsRootConstantBufferView(RP_COLLISIONDEBUG, debugCBAddress);

			D3DDebug::GetInstance()->GetDebugMesh(i)->Render(mCommandList.Get());

			++debugCBIndex;
		}
	}
}

void D3DFramework::RenderWidget(ID3D12GraphicsCommandList* cmdList)
{
	auto widgetPool = mCurrentFrameResource->mWidgetPool.get();
	UINT widgetObjIndex = 0;

	for (const auto& widget : mWidgets)
	{
		if (!widget->GetIsVisible())
			continue;

		auto widgetAddressStart = widgetPool->GetBuffer()->GetResource()->GetGPUVirtualAddress();
		D3D12_GPU_VIRTUAL_ADDRESS widgetCBAddress = widgetAddressStart + widgetObjIndex * widgetCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(RP_WIDGET, widgetCBAddress);

		widget->GetMesh()->Render(cmdList, 1, false);

		++widgetObjIndex;
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

#if defined(DEBUG) || defined(_DEBUG)
	XMVECTOR rayEnd = rayOrigin + rayDir * distance;
	D3DDebug::GetInstance()->DrawLine(rayOrigin, rayEnd);
#endif
	
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
