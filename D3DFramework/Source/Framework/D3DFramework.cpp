#include "../PrecompiledHeader/pch.h"
#include "D3DFramework.h"
#include "InputManager.h"
#include "AssetManager.h"
#include "FrameResource.h"
#include "UploadBuffer.h"
#include "GameTimer.h"
#include "Camera.h"
#include "InputManager.h"
#include "Octree.h"
#include "Ssao.h"
#include "Ssr.h"
#include "StopWatch.h"
#include "Random.h"
#include "BlurFilter.h"
#include "D3DDebug.h"
#include "ShadowMap.h"
#include "Physics.h"

#include "../Component/Material.h"
#include "../Component/Mesh.h"
#include "../Component/Widget.h"
#include "../Component/Sound.h"

#include "../Object/GameObject.h"
#include "../Object/DirectionalLight.h"
#include "../Object/PointLight.h"
#include "../Object/SpotLight.h"
#include "../Object/Particle.h"
#include "../Object/Billboard.h"
#include "../Object/SkySphere.h"
#include "../Object/Terrain.h"

#include <functional>

#ifdef PIX
#include <pix3.h>
#endif

using namespace std::literals;
using Microsoft::WRL::ComPtr;

D3DFramework::D3DFramework(HINSTANCE hInstance, const INT32 screenWidth, const INT32 screenHeight, 
	const std::wstring applicationName, const bool useWinApi)
	: D3DApp(hInstance, screenWidth, screenHeight, applicationName, useWinApi)
{
	// ���� �ϳ��� Framework���� �����Ѵ�.
	assert(instance == nullptr);
	instance = this;

	mainPassCB = std::make_unique<PassConstants>();
	for (UINT i = 0; i < LIGHT_NUM; ++i)
		shadowPassCB[i] = std::make_unique<PassConstants>();

	camera = std::make_unique<Camera>();
}

D3DFramework::~D3DFramework() 
{
	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
		frameResources[i] = nullptr;
	currentFrameResource = nullptr;

	for (int i = 0; i < (int)RenderLayer::Count; ++i)
		renderableObjects[i].clear();
	gameObjects.clear();
	lights.clear();
	widgets.clear();
	particles.clear();

	for (int i = 0; i < LIGHT_NUM; ++i)
		shadowPassCB[i] = nullptr;
	mainPassCB = nullptr;
	camera = nullptr;
	octreeRoot = nullptr;
	ssao = nullptr;
	ssr = nullptr;
	blurFilter = nullptr;
}

D3DFramework* D3DFramework::GetInstance()
{
	return instance;
}

Camera* D3DFramework::GetCamera() const
{
	return camera.get();
}

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

	for (UINT32 i = 0; i < FrameResource::processorCoreNum; ++i)
	{
		CloseHandle(workerBeginFrameEvents[i]);
		CloseHandle(workerFinishedFrameEvents[i]);
		workerThread[i].join();
	}
}

void D3DFramework::OnResize(const INT32 screenWidth, const INT32 screenHeight)
{
	__super::OnResize(screenWidth, screenHeight);

	// ī�޶� �� ����� �ٽ� ����Ѵ�.
	camera->SetLens(60.0f, GetAspectRatio(), 0.1f, 1000.0f);

	// ȭ�� �ػ󵵰� �ٲ�� ������ ũ�⵵ �ٲ��� �Ѵ�.
	for (auto& widget : widgets)
		widget->UpdateNumFrames();

#ifdef SSAO
	if(ssao)
		ssao->OnResize(d3dDevice.Get(), screenWidth, screenHeight);
#endif
	
#ifdef SSR
	if (ssr)
		ssr->OnResize(d3dDevice.Get(), screenWidth, screenHeight);
	if (blurFilter)
		blurFilter->OnResize(d3dDevice.Get(), screenWidth, screenHeight);
#endif
}

void D3DFramework::InitFramework()
{
	// ��ɾ� �Ҵ��ڿ� ����Ʈ�� �����Ѵ�.
	Reset(mainCommandList.Get(), mainCommandAlloc.Get());

	// �÷��̾� ī�޶� 3D ������ ������ �ǵ��� 
	// �����ʸ� �����ϰ� ���� ��ġ�� �����Ѵ�.
	camera->SetListener(listener.Get());
	camera->SetPosition(25.8f, -30.5f, 0.74f);
	camera->RotateY(-15.0f);
	camera->Pitch(-3.0f);

	// �ʿ��� �ּµ��� �ε��Ѵ�.
	AssetManager::GetInstance()->Initialize(d3dDevice.Get(), mainCommandList.Get(), d3dSound.Get());

#ifdef SSAO
	ssao = std::make_unique<Ssao>(d3dDevice.Get(), mainCommandList.Get(), screenWidth / 2, screenHeight / 2);
#endif

#ifdef SSR
	ssr = std::make_unique<Ssr>(d3dDevice.Get(), screenWidth / 2, screenHeight / 2);
	blurFilter = std::make_unique<BlurFilter>(d3dDevice.Get(), screenWidth / 2, screenHeight / 2, Ssr::ssrMapFormat);
#endif

	// �����ӿ�ũ�� ���Ǵ� ��� ��ü�� �̸� �����.
	CreateObjects();
	CreateLights();
	CreateWidgets(d3dDevice.Get(), mainCommandList.Get());
	CreateParticles();
	CreateTerrain();
	CreateFrameResources(d3dDevice.Get());
	CreateThreads();

	const UINT32 textureNum = (UINT32)AssetManager::GetInstance()->textures.size();
	const UINT32 shadowMapNum = (UINT32)LIGHT_NUM;
	const UINT32 particleNum = (UINT32)particles.size();

	CreateRootSignatures(textureNum, shadowMapNum);
	CreateDescriptorHeaps(textureNum, shadowMapNum, particleNum);
	CreatePSOs();
	CreateTerrainStdDevAndNormalMap();

	// �ʱ�ȭ ��ɵ��� �����Ѵ�.
	ThrowIfFailed(mainCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mainCommandList.Get() };
	commandQueue->ExecuteCommandLists(1, cmdLists);

#if defined(DEBUG) || defined(_DEBUG)
	D3DDebug::GetInstance()->CreateCommandObjects(d3dDevice.Get());
#endif

	// �ʱ�ȭ�� ���� ������ ��ٸ���.
	FlushCommandQueue();

	// �޽����� ���� ���ε� ���۸� �����Ѵ�.
	AssetManager::GetInstance()->DisposeUploaders();

	for (const auto& obj : gameObjects)
	{
		obj->BeginPlay();
	}

	// �浹�� ����ȭ�ϱ� ���� ����Ʈ���� �����Ѵ�.
	BoundingBox octreeAABB = BoundingBox(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(500.0f, 500.0f, 500.0f));
	octreeRoot = std::make_unique<Octree>(octreeAABB, gameObjects);
	octreeRoot->BuildTree();

	AssetManager::GetInstance()->sounds["WinterWind"]->SetPosition(10.0f, -35.0f, 30.0f);
	AssetManager::GetInstance()->sounds["WinterWind"]->Play(true);
}

void D3DFramework::CreateDescriptorHeaps(const UINT32 textureNum, const UINT32 shadowMapNum, const UINT32 particleNum)
{
	D3DApp::CreateDescriptorHeaps(textureNum, shadowMapNum, particleNum);

	DescriptorIndex::ssaoMapHeapIndex = DescriptorIndex::depthBufferHeapIndex + 1;
#ifdef SSAO
	ssao->BuildDescriptors(d3dDevice.Get(),
		GetGpuSrv(DescriptorIndex::deferredBufferHeapIndex + 4), // Normalx Map, Depth Map
		GetGpuSrv(DescriptorIndex::deferredBufferHeapIndex + 2), // Position Map
		GetCpuSrv(DescriptorIndex::ssaoMapHeapIndex),
		GetGpuSrv(DescriptorIndex::ssaoMapHeapIndex),
		GetRtv(SWAP_CHAIN_BUFFER_COUNT + DEFERRED_BUFFER_COUNT));
#endif

	DescriptorIndex::ssrMapHeapIndex = DescriptorIndex::ssaoMapHeapIndex + 3;
#ifdef SSR
	ssr->BuildDescriptors(d3dDevice.Get(),
		GetGpuSrv(DescriptorIndex::deferredBufferHeapIndex + 2), // Position Map
		GetGpuSrv(DescriptorIndex::deferredBufferHeapIndex + 4), // Normal Map x
		GetCpuSrv(DescriptorIndex::ssrMapHeapIndex),
		GetGpuSrv(DescriptorIndex::ssrMapHeapIndex),
		GetRtv(SWAP_CHAIN_BUFFER_COUNT + DEFERRED_BUFFER_COUNT + 2)); // Ssao�� �� ���� ����Ÿ��
	blurFilter->BuildDescriptors(d3dDevice.Get(),
		GetCpuSrv(DescriptorIndex::ssrMapHeapIndex + 1), 
		GetGpuSrv(DescriptorIndex::ssrMapHeapIndex + 1));
#endif


	DescriptorIndex::shadowMapHeapIndex = DescriptorIndex::ssrMapHeapIndex + 5; // Ssr Map, BlurFilter Srv, Uav
	UINT32 i = 0;
	for (const auto& light : lights)
	{
		light->GetShadowMap()->BuildDescriptors(d3dDevice.Get(),
			GetCpuSrv(DescriptorIndex::shadowMapHeapIndex + i),
			GetGpuSrv(DescriptorIndex::shadowMapHeapIndex + i),
			GetDsv(1));
		++i;
	}

	DescriptorIndex::particleHeapIndex = DescriptorIndex::shadowMapHeapIndex + shadowMapNum;
	i = 0;
	for (const auto& particle : particles)
	{
		particle->CreateBuffers(d3dDevice.Get(), GetCpuSrv(DescriptorIndex::particleHeapIndex + i * 3));
		++i;
	}

	DescriptorIndex::terrainHeapIndex = DescriptorIndex::particleHeapIndex + particleNum * 3;
	terrain->BuildDescriptors(d3dDevice.Get(),
		GetCpuSrv(DescriptorIndex::terrainHeapIndex),
		GetGpuSrv(DescriptorIndex::terrainHeapIndex));
}

void D3DFramework::SetDefaultState(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->RSSetViewports(1, &screenViewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvUavDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootSignature(rootSignatures["Common"].Get());

	// �� ��鿡 ���Ǵ� Pass ��� ���۸� ���´�.
	cmdList->SetGraphicsRootConstantBufferView((int)RpCommon::Pass, currentFrameResource->GetPassVirtualAddress());

	// ������ ���۴� ���� �����ϰ� �׳� �ϳ��� ��Ʈ �����ڷ� ���� �� �ִ�.
	cmdList->SetGraphicsRootShaderResourceView((int)RpCommon::Light, currentFrameResource->GetLightVirtualAddress());
	cmdList->SetGraphicsRootShaderResourceView((int)RpCommon::Material, currentFrameResource->GetMaterialVirtualAddress());

	// �� ��鿡 ���Ǵ� ��� �ؽ�ó�� ���´�. ���̺��� ù �����ڸ� ������
	// ���̺� �� ���� �����ڰ� �ִ����� Root Signature�� �����Ǿ� �ִ�.
	cmdList->SetGraphicsRootDescriptorTable((int)RpCommon::Texture, GetGpuSrv(DescriptorIndex::textureHeapIndex));

	// G-Buffer�� ���� ���� Ÿ�ٵ��� ���������ο� ���ε��Ѵ�.
	cmdList->SetGraphicsRootDescriptorTable((int)RpCommon::GBuffer, GetGpuSrv(DescriptorIndex::deferredBufferHeapIndex));

	// �׸��� ���� ���������ο� ���ε��Ѵ�.
	cmdList->SetGraphicsRootDescriptorTable((int)RpCommon::ShadowMap, GetGpuSrv(DescriptorIndex::shadowMapHeapIndex));

#ifdef SSAO
	// Ssao Map�� ���ε��Ѵ�. ��, Ssao�� ����� ��쿡�� ���ε��Ѵ�.
	cmdList->SetGraphicsRootDescriptorTable((int)RpCommon::Ssao, GetGpuSrv(DescriptorIndex::ssaoMapHeapIndex));
#endif

#ifdef SSR
	// Ssr Map�� ���ε��Ѵ�. ��, Ssao�� ����� ��쿡�� ���ε��Ѵ�.
	cmdList->SetGraphicsRootDescriptorTable((int)RpCommon::Ssr, GetGpuSrv(DescriptorIndex::ssrMapHeapIndex));
#endif
}

void D3DFramework::Render()
{
	__super::Render();

	// ������ ������ �������� �������Ѵ�.
	if (GetOptionEnabled(Option::Wireframe))
	{
		auto cmdList = currentFrameResource->frameCmdLists[0].Get();
		auto cmdAlloc = currentFrameResource->frameCmdAllocs[0].Get();
		Reset(cmdList, cmdAlloc);

		WireframePass(cmdList);

		ThrowIfFailed(cmdList->Close());
		ID3D12CommandList* cmdLists[] = { cmdList };
		commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	}
	// fill �������� �������Ѵ�.
	else
	{
#ifdef MULTITHREAD_RENDERING
		auto cmdListPre = currentFrameResource->frameCmdLists[0].Get();
		auto cmdListPost = currentFrameResource->frameCmdLists[1].Get();
	
		Init(cmdListPre);
		ParticleUpdate(cmdListPre);
		ShadowMapPass(cmdListPre);

		// �� ù��° ��ɾ���� ���� �����Ѵ�.
		ThrowIfFailed(cmdListPre->Close());
		ID3D12CommandList* commandListsPre[] = { cmdListPre };
		commandQueue->ExecuteCommandLists(_countof(commandListsPre), commandListsPre);

		// ��Ƽ ������ �������� ���, �ٸ� ��������� �������� �����Ѵ�.
		GBufferPass(nullptr);

		// G���۸� ä��� �������� �����Ѵ�.
		commandQueue->ExecuteCommandLists((UINT)currentFrameResource->executableCmdLists.size(), 
			currentFrameResource->executableCmdLists.data());

		DrawTerrain(cmdListPost, false);

		MidFrame(cmdListPost);
		LightingPass(cmdListPost);
		ForwardPass(cmdListPost);
		PostProcessPass(cmdListPost);

		Finish(cmdListPost);

		// ������ ��ɾ �����Ѵ�.
		ThrowIfFailed(cmdListPost->Close());
		ID3D12CommandList* commandListsPost[] = { cmdListPost };
		commandQueue->ExecuteCommandLists(_countof(commandListsPost), commandListsPost);
#else
		auto cmdList = currentFrameResource->frameCmdLists[0].Get();

		Init(cmdList);
		ParticleUpdate(cmdList);
		ShadowMapPass(cmdList);

		GBufferPass(nullptr);
		DrawTerrain(cmdList, false);

		MidFrame(cmdList);
		LightingPass(cmdList);
		ForwardPass(cmdList);
		PostProcessPass(cmdList);

		Finish(cmdList);

		ThrowIfFailed(cmdList->Close());
		ID3D12CommandList* commandLists[] = { cmdList };
		commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
#endif
	}

	// ���� ���ۿ� �ĸ� ���۸� �ٲ۴�.
	ThrowIfFailed(swapChain->Present(0, 0));
	currentBackBuffer = (currentBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

	// ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϵ��� ��Ÿ�� ���� ������Ų��.
	currentFrameResource->fence = ++currentFence;

	// �� ��Ÿ�� ������ �����ϴ� ���(Signal)�� ��� ��⿭�� �߰��Ѵ�.
	// �� ��Ÿ�� ������ GPU�� �� Signal() ��ɱ����� ��� ����� ó���ϱ�
	// �������� �������� �ʴ´�.
	commandQueue->Signal(fence.Get(), currentFence);
}

void D3DFramework::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	currentFrameResourceIndex = (currentFrameResourceIndex + 1) % NUM_FRAME_RESOURCES;
	currentFrameResource = frameResources[currentFrameResourceIndex].get();

	// GPU�� ���� FrameResource���� ����� �����ƴ��� Ȯ���Ѵ�.
	// �ƴ϶�� Fence Point�� ������ ������ ��ٸ���.
	if (currentFrameResource->fence != 0 && fence->GetCompletedValue() < currentFrameResource->fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(fence->SetEventOnCompletion(currentFrameResource->fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	if (InputManager::keys['w'] || InputManager::keys['W'])
		camera->Walk(deltaTime);

	if (InputManager::keys['s'] || InputManager::keys['S'])
		camera->Walk(-deltaTime);

	if (InputManager::keys['a'] || InputManager::keys['A'])
		camera->Strafe(-deltaTime);

	if (InputManager::keys['d'] || InputManager::keys['D'])
		camera->Strafe(deltaTime);

	camera->UpdateViewMatrix();

	// ī�޶� ���� ���������� ���� ��ǥ��� ��ȯ�Ѵ�.
	worldCamFrustum = camera->GetWorldCameraBounding();

	// �� ��� ���۸� ������Ʈ�Ѵ�.
	octreeRoot->Update(deltaTime);
	UpdateObjectBuffer(deltaTime);
	UpdateLightBuffer(deltaTime);
	UpdateMaterialBuffer(deltaTime);
	UpdateMainPassBuffer(deltaTime);
	UpdateWidgetBuffer(deltaTime);
	UpdateParticleBuffer(deltaTime);
	UpdateTerrainBuffer(deltaTime);
	UpdateSsrBuffer(deltaTime);

#ifdef SSAO
	UpdateSsaoBuffer(deltaTime);
#endif

#if defined(DEBUG) || defined(_DEBUG)
	D3DDebug::GetInstance()->Update(deltaTime);
#endif
}


void D3DFramework::UpdateObjectBuffer(float deltaTime)
{
	auto currObjectCB = currentFrameResource->objectPool->GetBuffer();
	UINT32 objectIndex = 0;

	for (auto& obj : gameObjects)
	{
		obj->Tick(deltaTime);

		if (objectIndex != obj->cbIndex)
		{
			obj->cbIndex = objectIndex;
			obj->UpdateNumFrames();
		}

		if (obj->IsUpdate())
		{
			// ������Ʈ�� ��� ���۸� ������Ʈ�Ѵ�.
			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(obj->GetWorld()));
			objConstants.materialIndex = obj->GetMaterial()->GetMaterialIndex();

			currObjectCB->CopyData(objectIndex, objConstants);
		}
		++objectIndex;
	}
}

void D3DFramework::UpdateLightBuffer(float deltaTime)
{
	// NDC ���� [-1, 1]^2�� �ؽ�ó ���� [0, 1]^2���� ��ȯ�ϴ� ���
	static XMMATRIX toTextureTransform(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	auto currLightBuffer = currentFrameResource->lightBufferPool->GetBuffer();
	auto passCB = currentFrameResource->passPool->GetBuffer();
	UINT32 i = 0;
	for (const auto& light : lights)
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

			XMStoreFloat4x4(&shadowPassCB[i]->view, XMMatrixTranspose(view));
			XMStoreFloat4x4(&shadowPassCB[i]->invView, XMMatrixTranspose(invView));
			XMStoreFloat4x4(&shadowPassCB[i]->proj, XMMatrixTranspose(proj));
			XMStoreFloat4x4(&shadowPassCB[i]->invProj, XMMatrixTranspose(invProj));
			XMStoreFloat4x4(&shadowPassCB[i]->viewProj, XMMatrixTranspose(viewProj));
			XMStoreFloat4x4(&shadowPassCB[i]->invViewProj, XMMatrixTranspose(invViewProj));
			shadowPassCB[i]->eyePosW = light->GetPosition();
			shadowPassCB[i]->renderTargetSize = XMFLOAT2((float)SHADOW_MAP_SIZE, (float)SHADOW_MAP_SIZE);
			shadowPassCB[i]->invRenderTargetSize = XMFLOAT2(1.0f / SHADOW_MAP_SIZE, 1.0f / SHADOW_MAP_SIZE);
			shadowPassCB[i]->nearZ = light->falloffStart;
			shadowPassCB[i]->farZ = light->falloffEnd;

			currLightBuffer->CopyData(i, lightData);
			passCB->CopyData(1 + i, *shadowPassCB[i]);

			light->DecreaseNumFrames();
		}
		++i;
	}
}

void D3DFramework::UpdateMaterialBuffer(float deltaTime)
{
	auto currMaterialBuffer = currentFrameResource->materialBufferPool->GetBuffer();
	for (const auto& e : AssetManager::GetInstance()->materials)
	{
		Material* mat = e.second.get();
		mat->Tick(deltaTime);
		if (mat->IsUpdate())
		{
			MaterialData matData;

			matData.diffuseAlbedo = mat->diffuse;
			matData.specular = mat->specular;
			matData.roughness = mat->roughness;
			matData.diffuseMapIndex = mat->diffuseMapIndex;
			matData.normalMapIndex = mat->normalMapIndex;
			matData.heightMapIndex = mat->heightMapIndex;
			DirectX::XMStoreFloat4x4(&matData.matTransform, XMMatrixTranspose(mat->GetMaterialTransform()));

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

	XMMATRIX view = camera->GetView();
	XMMATRIX proj = camera->GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mainPassCB->view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mainPassCB->invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mainPassCB->proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mainPassCB->invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mainPassCB->viewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mainPassCB->invViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mainPassCB->projTex, XMMatrixTranspose(proj * T));
	XMStoreFloat4x4(&mainPassCB->viewProjTex, XMMatrixTranspose(viewProj * T));
	mainPassCB->ambientLight = { 0.4f, 0.4f, 0.6f, 1.0f };
	mainPassCB->renderTargetSize = XMFLOAT2((float)screenWidth, (float)screenHeight);
	mainPassCB->invRenderTargetSize = XMFLOAT2(1.0f / screenWidth, 1.0f / screenHeight);
	mainPassCB->eyePosW = camera->GetPosition3f();
	mainPassCB->nearZ = camera->GetNearZ();
	mainPassCB->farZ = camera->GetFarZ();
	mainPassCB->totalTime = gameTimer->GetTotalTime();
	mainPassCB->deltaTime = gameTimer->GetDeltaTime();
	mainPassCB->fogEnabled = GetOptionEnabled(Option::Fog);
	mainPassCB->fogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	mainPassCB->fogStart = 0.0f;
	mainPassCB->fogRange = 100.0f;
	mainPassCB->fogDensity = 0.1f;
	mainPassCB->fogType = (std::uint32_t)FogType::Exponential;

	auto passCB = currentFrameResource->passPool->GetBuffer();
	passCB->CopyData(0, *mainPassCB);
}

void D3DFramework::UpdateWidgetBuffer(float deltaTime)
{
	// [0, 1]������ ������ ������ ���� ��ġ�� ���� ��ǥ���� [-1, 1]������ ��ȯ�Ѵ�.
	static std::function<float(float)> transformHomogenous = [](float x) { return x * 2.0f - 1.0f; };

	auto currWidgetCB = currentFrameResource->widgetPool->GetBuffer();
	auto& currWidgetVBs = currentFrameResource->widgetVBs;
	UINT32 widgetIndex = 0;

	for (const auto& widget : widgets)
	{
		widget->Tick(deltaTime);
		if (widgetIndex != widget->cbIndex)
		{
			widget->cbIndex = widgetIndex;
			widget->UpdateNumFrames();
		}

		if (widget->IsUpdate())
		{
			ObjectConstants objConstants;
			objConstants.materialIndex = widget->GetMaterial()->diffuseMapIndex;
			currWidgetCB->CopyData(widgetIndex, objConstants);

			float posX = (float)widget->posX;
			float posY = (float)widget->posY;
			float width = (float)widget->width;
			float height = (float)widget->height;
			float anchorX = widget->anchorX;
			float anchorY = widget->anchorY;

			float minX = anchorX + (posX / screenWidth);
			float minY = anchorY + (posY / screenHeight);
			float maxX = anchorX + ((posX + width) / screenWidth);
			float maxY = anchorY + ((posY + height) / screenHeight);

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
				4, (UINT32)sizeof(WidgetVertex));

			widget->DecreaseNumFrames();
		}
		++widgetIndex;
	}
}

void D3DFramework::UpdateParticleBuffer(float deltaTime)
{
	auto currParticleCB = currentFrameResource->particlePool->GetBuffer();
	UINT32 particleIndex = 0;

	for (const auto& particle : particles)
	{
		particle->Tick(deltaTime);

		if (particleIndex != particle->cbIndex)
		{
			particle->cbIndex = particleIndex;
			particle->UpdateNumFrames();
		}

		if (particle->IsUpdate())
		{
			ParticleConstants particleConstants;
			particle->SetParticleConstants(particleConstants);
			currParticleCB->CopyData(particleIndex, particleConstants);

			particle->DecreaseNumFrames();
		}

		++particleIndex;
	}
}

void D3DFramework::UpdateSsaoBuffer(float deltaTime)
{
	static UINT32 isSsaoCBUpdate = NUM_FRAME_RESOURCES;
	static const XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	if (isSsaoCBUpdate > 0)
	{
		SsaoConstants ssaoCB;

		ssao->GetOffsetVectors(ssaoCB.offsetVectors);
		auto blurWeights = ssao->CalcGaussWeights(2.5f);
		ssaoCB.blurWeights[0] = XMFLOAT4(&blurWeights[0]);
		ssaoCB.blurWeights[1] = XMFLOAT4(&blurWeights[4]);
		ssaoCB.blurWeights[2] = XMFLOAT4(&blurWeights[8]);

		ssaoCB.occlusionRadius = 0.5f;
		ssaoCB.occlusionFadeStart = 0.2f;
		ssaoCB.occlusionFadeEnd = 1.0f;
		ssaoCB.surfaceEpsilon = 0.05f;

		ssaoCB.renderTargetInvSize = DirectX::XMFLOAT2(1.0f / ssao->GetWidth(), 1.0f / ssao->GetHeight());
		ssaoCB.ssaoContrast = 2.0f;

		auto currSsaoCB = currentFrameResource->ssaoPool->GetBuffer();
		currSsaoCB->CopyData(0, ssaoCB);

		--isSsaoCBUpdate;
	}
}

void D3DFramework::UpdateTerrainBuffer(float deltaTime)
{
	auto currTerrainCB = currentFrameResource->terrainPool->GetBuffer();

	if (terrain->IsUpdate())
	{
		TerrainConstants terrainConstants;
		XMStoreFloat4x4(&terrainConstants.terrainWorld, XMMatrixTranspose(terrain->GetWorld()));
		terrainConstants.pixelDimesion = terrain->GetPixelDimesion();
		terrainConstants.geometryDimension = terrain->GetGeometryDimesion();
		terrainConstants.minLOD = 16.0f;
		terrainConstants.maxLOD = 64.0f;
		terrainConstants.minDistance = 300.0f;
		terrainConstants.maxDistance = 1000.0f;
		terrainConstants.heightScale = 100.0f;
		terrainConstants.texScale = 10.0f;
		terrainConstants.materialIndex = terrain->GetMaterial()->GetMaterialIndex();
		currTerrainCB->CopyData(0, terrainConstants);

		terrain->DecreaseNumFrames();
	}
}

void D3DFramework::UpdateSsrBuffer(float deltaTime)
{
	static UINT32 isSsrCBUpdate = NUM_FRAME_RESOURCES;
	auto currSsrCB = currentFrameResource->ssrPool->GetBuffer();

	if (isSsrCBUpdate > 0)
	{
		SsrConstants ssrConstants;
		ssrConstants.maxDistance = 40.0f;
		ssrConstants.thickness = 1.0f;
		ssrConstants.rayTraceStep = 20;
		ssrConstants.binaryStep = 5;
		ssrConstants.screenEdgeFadeStart = XMFLOAT2(100.0f, 50.0f);
		ssrConstants.strideCutoff = 2.0f;
		ssrConstants.resolutuon = 0.2f;
		currSsrCB->CopyData(0, ssrConstants);

		--isSsrCBUpdate;
	}
}

void D3DFramework::UpdateObjectBufferPool()
{
	const UINT32 allObjectCount = (UINT32)gameObjects.size() + (UINT32)widgets.size();

	if (currentFrameResource)
	{
		auto currObjectPool = currentFrameResource->objectPool.get();
		if (currObjectPool->GetBufferCount() < allObjectCount)
			currObjectPool->Resize(allObjectCount * 2);
	}
}

void D3DFramework::CreateObjects()
{
	std::shared_ptr<GameObject> object;
	std::shared_ptr<Billboard> tree;

	object = std::make_shared<SkySphere>("Sky"s);
	object->SetScale(5000.0f, 5000.0f, 5000.0f);
	object->SetRotation(0.0f, 90.0f, 0.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Sky"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("SkySphere"s));
	object->SetCollisionEnabled(false);
	renderableObjects[(int)RenderLayer::Sky].push_back(object);
	gameObjects.push_back(object);

	object = std::make_shared<GameObject>("Floor"s);
	object->SetPosition(50.0f, -40.0f, 60.0f);
	object->SetScale(200.0f, 2.5f, 200.0f);
	object->SetRotation(0.0f, 60.0f, 0.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Ice"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Cube_AABB"s));
	object->SetCollisionEnabled(true);
	renderableObjects[(int)RenderLayer::Opaque].push_back(object);
	gameObjects.push_back(object);

	object = std::make_shared<GameObject>("Sword"s);
	object->SetPosition(10.0f, -35.0f, 30.0f);
	object->SetScale(0.1f, 0.1f, 0.1f);
	object->SetRotation(30.0f, 5.0f, 0.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Sword"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Sword"s));
	object->SetCollisionEnabled(true);
	renderableObjects[(int)RenderLayer::Opaque].push_back(object);
	gameObjects.push_back(object);

	tree = std::make_shared<Billboard>("Tree1"s);
	tree->SetPosition(0.0f, 0.0f, 200.0f);
	tree->mSize = XMFLOAT2(40.0f, 40.0f);
	tree->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree1"s));
	tree->BuildBillboardMesh(d3dDevice.Get(), mainCommandList.Get());
	renderableObjects[(int)RenderLayer::Billborad].push_back(tree);
	gameObjects.push_back(tree);

	tree = std::make_shared<Billboard>("Tree1"s);
	tree->SetPosition(90.0f, -10.0f, 170.0f);
	tree->mSize = XMFLOAT2(40.0f, 40.0f);
	tree->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree1"s));
	tree->BuildBillboardMesh(d3dDevice.Get(), mainCommandList.Get());
	renderableObjects[(int)RenderLayer::Billborad].push_back(tree);
	gameObjects.push_back(tree);

	tree = std::make_shared<Billboard>("Tree1"s);
	tree->SetPosition(-180.0f, 30.0f, 230.0f);
	tree->mSize = XMFLOAT2(60.0f, 60.0f);
	tree->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree1"s));
	tree->BuildBillboardMesh(d3dDevice.Get(), mainCommandList.Get());
	renderableObjects[(int)RenderLayer::Billborad].push_back(tree);
	gameObjects.push_back(tree);

	tree = std::make_shared<Billboard>("Tree2"s);
	tree->SetPosition(-50.0f, 5.0f, 200.0f);
	tree->mSize = XMFLOAT2(40.0f, 40.0f);
	tree->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree2"s));
	tree->BuildBillboardMesh(d3dDevice.Get(), mainCommandList.Get());
	renderableObjects[(int)RenderLayer::Billborad].push_back(tree);
	gameObjects.push_back(tree);

	tree = std::make_shared<Billboard>("Tree2"s);
	tree->SetPosition(30.0f, -10.0f, 150.0f);
	tree->mSize = XMFLOAT2(60.0f, 60.0f);
	tree->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree2"s));
	tree->BuildBillboardMesh(d3dDevice.Get(), mainCommandList.Get());
	renderableObjects[(int)RenderLayer::Billborad].push_back(tree);
	gameObjects.push_back(tree);

	tree = std::make_shared<Billboard>("Tree2"s);
	tree->SetPosition(-60.0f, -10.0f, 100.0f);
	tree->mSize = XMFLOAT2(30.0f, 30.0f);
	tree->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree2"s));
	tree->BuildBillboardMesh(d3dDevice.Get(), mainCommandList.Get());
	renderableObjects[(int)RenderLayer::Billborad].push_back(tree);
	gameObjects.push_back(tree);

	tree = std::make_shared<Billboard>("Tree3"s);
	tree->SetPosition(80.0f, 10.0f, 210.0f);
	tree->mSize = XMFLOAT2(30.0f, 30.0f);
	tree->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree3"s));
	tree->BuildBillboardMesh(d3dDevice.Get(), mainCommandList.Get());
	renderableObjects[(int)RenderLayer::Billborad].push_back(tree);
	gameObjects.push_back(tree);

	tree = std::make_shared<Billboard>("Tree3"s);
	tree->SetPosition(-100.0f, 20.0f, 100.0f);
	tree->mSize = XMFLOAT2(50.0f, 50.0f);
	tree->SetMaterial(AssetManager::GetInstance()->FindMaterial("Tree3"s));
	tree->BuildBillboardMesh(d3dDevice.Get(), mainCommandList.Get());
	renderableObjects[(int)RenderLayer::Billborad].push_back(tree);
	gameObjects.push_back(tree);

	object = std::make_shared<GameObject>("Rock1"s);
	object->SetPosition(30.0f, -39.0f, 30.0f);
	object->SetScale(3.0f, 3.0f, 3.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Rock1"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Rock1"s));
	object->SetCollisionEnabled(true);
	renderableObjects[(int)RenderLayer::Opaque].push_back(object);
	gameObjects.push_back(object);

	object = std::make_shared<GameObject>("Rock1"s);
	object->SetPosition(26.0f, -39.5f, 42.0f);
	object->SetScale(2.5f, 2.5f, 2.5f);
	object->SetRotation(2.0f, 10.0f, 0.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Rock1"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Rock1"s));
	object->SetCollisionEnabled(true);
	renderableObjects[(int)RenderLayer::Opaque].push_back(object);
	gameObjects.push_back(object);

	object = std::make_shared<GameObject>("Rock1"s);
	object->SetPosition(27.5f, -40.0f, 56.0f);
	object->SetScale(2.8f, 2.8f, 2.8f);
	object->SetRotation(8.0f, 30.0f, 6.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Rock1"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Rock1"s));
	object->SetCollisionEnabled(true);
	renderableObjects[(int)RenderLayer::Opaque].push_back(object);
	gameObjects.push_back(object);

	object = std::make_shared<GameObject>("Rock1"s);
	object->SetPosition(32.0f, -40.0f, 75.0f);
	object->SetScale(3.8f, 3.8f, 3.8f);
	object->SetRotation(43.0f, 60.0f, 15.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Rock1"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Rock1"s));
	object->SetCollisionEnabled(true);
	renderableObjects[(int)RenderLayer::Opaque].push_back(object);
	gameObjects.push_back(object);

	object = std::make_shared<GameObject>("Rock1"s);
	object->SetPosition(20.0f, -39.0f, 86.0f);
	object->SetScale(2.6f, 2.6f, 2.6f);
	object->SetRotation(0.0f, 78.0f, 0.0f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Rock1"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Rock1"s));
	object->SetCollisionEnabled(true);
	renderableObjects[(int)RenderLayer::Opaque].push_back(object);
	gameObjects.push_back(object);

	object = std::make_shared<GameObject>("Rock1"s);
	object->SetPosition(9.0f, -40.0f, 100.0f);
	object->SetScale(2.9f, 2.9f, 2.9f);
	object->SetRotation(2.0f, 60.0f, 0.2f);
	object->SetMaterial(AssetManager::GetInstance()->FindMaterial("Rock1"s));
	object->SetMesh(AssetManager::GetInstance()->FindMesh("Rock1"s));
	object->SetCollisionEnabled(true);
	renderableObjects[(int)RenderLayer::Opaque].push_back(object);
	gameObjects.push_back(object);
}

void D3DFramework::CreateTerrain()
{
	terrain = std::make_shared<Terrain>("Terrian"s);
	terrain->SetPosition(0.0f, -50.0f, 0.0f);
	terrain->SetScale(10.0f, 10.0f, 10.0f);
	terrain->BuildMesh(d3dDevice.Get(), mainCommandList.Get(), 100.0f, 100.0f, 8, 8);
	terrain->SetMaterial(AssetManager::GetInstance()->FindMaterial("Terrain"s));
	terrain->CalculateWorld();
	renderableObjects[(int)RenderLayer::Terrain].push_back(terrain);
}

void D3DFramework::CreateLights()
{
	std::shared_ptr<Light> light;

	light = std::make_shared<DirectionalLight>("DirectionalLight"s, d3dDevice.Get());
	light->SetPosition(50.0f, 10.0f, 30.0f);
	light->Rotate(45.0f, -90.0f, 0.0f);
	light->strength = { 0.8f, 0.8f, 0.8f };
	light->falloffStart = 0.5f;
	light->falloffEnd = 75.0f;
	light->shadowMapSize = { 50.0f, 50.0f };
	light->spotAngle = 45.0f;
	lights.push_back(std::move(light));

	if ((UINT)lights.size() != LIGHT_NUM)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "Max Light Num Difference Error" << std::endl;
#endif
	}
}

void D3DFramework::CreateWidgets(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	std::shared_ptr<Widget> widget;

	widget = std::make_shared<Widget>("DiffuseMapDebug"s, device, cmdList);
	widget->SetPosition(0, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("SpecularMapDebug"s, device, cmdList);
	widget->SetPosition(160, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("RoughnessMapDebug"s, device, cmdList);
	widget->SetPosition(320, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("NormalMapDebug"s, device, cmdList);
	widget->SetPosition(480, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("DetphMapDebug"s, device, cmdList);
	widget->SetPosition(640, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widgets.push_back(std::move(widget));
	
	widget = std::make_shared<Widget>("SsaoMapDebug"s, device, cmdList);
	widget->SetPosition(800, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widgets.push_back(std::move(widget));

	widget = std::make_shared<Widget>("SsrMapDebug"s, device, cmdList);
	widget->SetPosition(960, -120);
	widget->SetSize(160, 120);
	widget->SetAnchor(0.0f, 1.0f);
	widget->SetMaterial(AssetManager::GetInstance()->FindMaterial("Default"s));
	widgets.push_back(std::move(widget));
}

void D3DFramework::CreateParticles()
{
	std::shared_ptr<Particle> particle;

	ParticleData start;
	start.lifeTime = 7.5f;
	start.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	start.size = XMFLOAT2(1.0f, 1.0f);
	start.speed = 30.0f;

	ParticleData end;
	end.color = XMFLOAT4(0.6f, 0.6f, 0.8f, 0.1f);
	end.size = XMFLOAT2(0.25f, 0.25f);
	end.speed = 10.0f;

	particle = std::make_unique<Particle>("Snow"s, 50000);
	particle->start = start;
	particle->end = end;
	particle->spawnTimeRange = std::make_pair<float, float>(0.2f, 0.2f);
	particle->emitNum = 500;
	particle->isInfinite = true;
	particle->enabledGravity = true;
	particle->SetPosition(10.0f, 40.0f, 20.0f);
	particle->SetMaterial(AssetManager::GetInstance()->FindMaterial("Radial_Gradient"s));
	particles.push_back(std::move(particle));
}

void D3DFramework::CreateFrameResources(ID3D12Device* device)
{
	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		bool isMultithreadRendering;
#ifdef MULTITHREAD_RENDERING
		isMultithreadRendering = true;
#else
		isMultithreadRendering = false;
#endif
		// ������ �ڿ��� �����Ѵ�.
		frameResources[i] = std::make_unique<FrameResource>(device, isMultithreadRendering,
			1 + LIGHT_NUM, (UINT)gameObjects.size() * 2, LIGHT_NUM,
			(UINT)AssetManager::GetInstance()->materials.size(), (UINT)widgets.size(), (UINT)particles.size());

		// ���� ���� ���۴� ���� �����Ѵ�.
		frameResources[i]->widgetVBs.reserve((UINT)widgets.size());
		for (const auto& widget : widgets)
		{
			std::unique_ptr<UploadBuffer<WidgetVertex>> vb = std::make_unique<UploadBuffer<WidgetVertex>>(device, 4, false);
			frameResources[i]->widgetVBs.push_back(std::move(vb));
		}
	}
}

void D3DFramework::CreateTerrainStdDevAndNormalMap()
{
	// Terrain���� ����� ǥ�� ���� LODMap�� ��� ���� �̸� ����Ѵ�.
	// �̵��� �� �� ����� �ʿ�� ����.

	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvUavDescriptorHeap.Get() };
	mainCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// ����Ʈ�ʿ� ���� ǥ�������� ����Ѵ�.
	mainCommandList->SetComputeRootSignature(rootSignatures["TerrainCompute"].Get());
	mainCommandList->SetComputeRootDescriptorTable((int)RpTerrainCompute::HeightMap,
		GetGpuSrv(DescriptorIndex::textureHeapIndex + terrain->GetMaterial()->heightMapIndex));
	terrain->SetUavDescriptors(mainCommandList.Get());

	// ǥ�������� ����Ͽ� LOD�� �����Ѵ�.
	mainCommandList->SetPipelineState(pipelineStateObjects["TerrainStdDev"].Get());
	terrain->LODCompute(mainCommandList.Get());

	// ����� ����Ͽ� ��� ���� �����.
	mainCommandList->SetPipelineState(pipelineStateObjects["TerrainNormal"].Get());
	terrain->NormalCompute(mainCommandList.Get());
}

void D3DFramework::RenderObject(ID3D12GraphicsCommandList* cmdList, Renderable* obj,
	D3D12_GPU_VIRTUAL_ADDRESS startAddress, BoundingFrustum* frustum) const
{
	obj->SetConstantBuffer(cmdList, startAddress);
	obj->Render(cmdList, frustum);
}

void D3DFramework::RenderObjects(ID3D12GraphicsCommandList* cmdList, const std::list<std::shared_ptr<Renderable>>& list,
	D3D12_GPU_VIRTUAL_ADDRESS startAddress, BoundingFrustum* frustum, const UINT32 threadIndex, const UINT32 threadNum) const
{
	UINT32 currentNum = threadIndex;
	UINT32 maxNum = (UINT32)list.size();
	if (list.empty() || currentNum >= maxNum)
		return;

	auto iter = list.begin();
	std::advance(iter, currentNum);
	while(true)
	{
		RenderObject(cmdList, (*iter).get(), startAddress, frustum);

		currentNum += threadNum;
		if (currentNum >= maxNum)
			return;
		std::advance(iter, threadNum);
	}
}

void D3DFramework::RenderActualObjects(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum)
{
	auto currObjectCB = currentFrameResource->objectPool->GetBuffer();
	D3D12_GPU_VIRTUAL_ADDRESS startAddress = currObjectCB->GetResource()->GetGPUVirtualAddress();

	RenderObjects(cmdList, renderableObjects[(int)RenderLayer::Opaque], startAddress, frustum);
	RenderObjects(cmdList, renderableObjects[(int)RenderLayer::AlphaTested], startAddress, frustum);
	RenderObjects(cmdList, renderableObjects[(int)RenderLayer::Billborad], startAddress, frustum);
	RenderObjects(cmdList, renderableObjects[(int)RenderLayer::Transparent], startAddress, frustum);
}

bool D3DFramework::Picking(HitInfo& hitInfo, const INT32 screenX, const INT32 screenY,
	const float distance, const bool isMeshCollision) const
{
	XMFLOAT4X4 proj = camera->GetProj4x4f();

	// Picking Ray�� �� �������� ����Ѵ�.
	float vx = (2.0f * screenX / screenWidth - 1.0f) / proj(0, 0);
	float vy = (-2.0f * screenY / screenHeight + 1.0f) / proj(1, 1);

	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	XMMATRIX view = camera->GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	// Picking Ray�� ���� �������� ��ȯ�Ѵ�.
	rayOrigin = XMVector3TransformCoord(rayOrigin, invView);
	rayDir = XMVector3TransformNormal(rayDir, invView);
	rayDir = XMVector3Normalize(rayDir);

	bool nearestHit = false;
	float nearestDist = FLT_MAX;
	GameObject* hitObj = nullptr;

#if defined(DEBUG) || defined(_DEBUG)
	D3DDebug::GetInstance()->DrawRay(rayOrigin, rayOrigin + (rayDir * distance));
#endif

	// Picking Ray�� �浹�� �˻��Ѵ�.
	for (const auto& obj : gameObjects)
	{
		float hitDist = FLT_MAX;
		bool isHit = Physics::IsCollision(obj.get(), rayOrigin, rayDir, hitDist, isMeshCollision);

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

	if (!nearestHit)
		return false;

	if (nearestHit)
	{
		std::cout << "Picking : " << hitObj->ToString() << std::endl;
	}

	if (nearestDist < distance)
	{
		hitInfo.dist = nearestDist;
		hitInfo.obj = (void*)hitObj;
		XMStoreFloat3(&hitInfo.rayOrigin, rayOrigin);
		XMStoreFloat3(&hitInfo.rayDirection, rayDir);

		return true;
	}

	return false;
}

GameObject* D3DFramework::FindGameObject(const std::string name)
{
	auto iter = std::find_if(gameObjects.begin(), gameObjects.end(),
		[&name](const std::shared_ptr<GameObject> obj) -> bool
	{ if (name.compare(obj->GetName()) == 0) return true; return false; });

	if (iter != gameObjects.end())
		return iter->get();

	return nullptr;
}

GameObject* D3DFramework::FindGameObject(const UINT64 uid)
{
	auto iter = std::find_if(gameObjects.begin(), gameObjects.end(),
		[&uid](const std::shared_ptr<GameObject> obj) -> bool
	{ if (uid == obj->GetUID()) return true; return false; });

	if (iter != gameObjects.end())
		return iter->get();

	return nullptr;
}

void D3DFramework::WireframePass(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ���� Ÿ�� �� ���� ���۸� ����� ��� ���� �ܰ迡 ���ε��Ѵ�.
	cmdList->ClearRenderTargetView(GetCurrentBackBufferView(), (float*)&backBufferClearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

	SetDefaultState(cmdList);

#ifdef PIX
	PIXBeginEvent(cmdList, 0, "Wirefrmae Rendering");
#endif

	// ȭ�鿡 ���̴� �ǰ�ü�鸸 �׸���.
	cmdList->SetPipelineState(pipelineStateObjects["Wireframe"].Get());
	RenderActualObjects(cmdList, &worldCamFrustum);

#ifdef PIX
	PIXEndEvent(cmdList);
#endif

	// ������ �׸���.
	DrawTerrain(cmdList, true);

	SetDefaultState(cmdList);

#if defined(DEBUG) || defined(_DEBUG)
	// ������� �޽����� �׸���.
	cmdList->SetPipelineState(pipelineStateObjects["Debug"].Get());
	D3DDebug::GetInstance()->Render(cmdList);
#endif

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void D3DFramework::ShadowMapPass(ID3D12GraphicsCommandList* cmdList)
{
	SetDefaultState(cmdList);

#ifdef PIX
	PIXBeginEvent(cmdList, 0, L"ShadowMap Rendering");
#endif

	cmdList->SetPipelineState(pipelineStateObjects["ShadowMap"].Get());
	UINT i = 0;
	for (const auto& light : lights)
	{
		// ������ �ʿ� ���� �н� ���۸� ���������ο� ���ε��Ѵ�.
		D3D12_GPU_VIRTUAL_ADDRESS shadowPassCBAddress = currentFrameResource->GetPassVirtualAddress() +
			(1 + i++) * ConstantsSize::passCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView((int)RpCommon::Pass, shadowPassCBAddress);

		// ������ ���� �׸���.
		light->RenderSceneToShadowMap(cmdList);
	}

#ifdef PIX
	PIXEndEvent(cmdList);
#endif
}

void D3DFramework::GBufferPass(ID3D12GraphicsCommandList* cmdList)
{
	for (UINT i = 0; i < FrameResource::processorCoreNum; ++i)
	{
		// �̺�Ʈ�� ��ȣ���°� �Ǹ� �� Worker �����忡�� ������Ʈ�� �����ϴ�
		// ����� �߰��ϱ� �����Ѵ�.
		SetEvent(workerBeginFrameEvents[i]);
	}
	// �� Worker �����尡 ��� ���� ����� ���ƴٸ� 
	// ��� ���¿��� Ǯ������ �ȴ�.
	WaitForMultipleObjects(FrameResource::processorCoreNum, workerFinishedFrameEvents.data(), TRUE, INFINITE);
}

void D3DFramework::LightingPass(ID3D12GraphicsCommandList* cmdList)
{
#ifdef SSAO
	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvUavDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

#ifdef PIX
	PIXBeginEvent(cmdList, 0, L"SSAO Rendering");
#endif

	// ssao���� �׸��� ����Ų��.
	cmdList->SetGraphicsRootSignature(rootSignatures["Ssao"].Get());
	ssao->ComputeSsao(cmdList, pipelineStateObjects["SsaoCompute"].Get(),
		currentFrameResource->GetSsaoVirtualAddress(), currentFrameResource->GetPassVirtualAddress());
	ssao->BlurAmbientMap(cmdList, pipelineStateObjects["SsaoBlur"].Get(), 2);

#ifdef PIX
	PIXEndEvent(cmdList);
#endif

#endif

	SetDefaultState(cmdList);

#ifdef PIX
	PIXBeginEvent(cmdList, 0, L"LightingPass Rendering");
#endif

	cmdList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, nullptr);
	cmdList->SetPipelineState(pipelineStateObjects["LightingPass"].Get());

	// G���۸� ����Ͽ� ȭ�鿡 ���Ǵ� �ȼ��� �������� ����Ѵ�.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

#ifdef PIX
	PIXEndEvent(cmdList);
#endif
}

void D3DFramework::ForwardPass(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

	SetDefaultState(cmdList);

#ifdef PIX
	PIXBeginEvent(cmdList, 0, L"Sky Rendering");
#endif

	// �ϴ��� �׸���.
	cmdList->SetPipelineState(pipelineStateObjects["Sky"].Get());
	RenderObjects(cmdList, renderableObjects[(int)RenderLayer::Sky], currentFrameResource->GetObjectVirtualAddress());

#ifdef PIX
	PIXEndEvent(cmdList);

	PIXBeginEvent(cmdList, 0, L"Particle Rendering");
#endif

	// ��ƼŬ �������� ���Ǵ� ���ҽ����� ���ε��Ѵ�.
	cmdList->SetGraphicsRootSignature(rootSignatures["ParticleRender"].Get());
	cmdList->SetPipelineState(pipelineStateObjects["ParticleRender"].Get());
	cmdList->SetGraphicsRootConstantBufferView((int)RpParticleGraphics::Pass, currentFrameResource->GetPassVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable((int)RpParticleGraphics::Texture, GetGpuSrv(DescriptorIndex::textureHeapIndex));
	for (const auto& particle : particles)
	{
		particle->SetBufferSrv(cmdList);

		// ��ƼŬ�� �׸���.
		RenderObject(cmdList, particle.get(), currentFrameResource->GetParticleVirtualAddress());
	}

#ifdef PIX
	PIXEndEvent(cmdList);
#endif

	SetDefaultState(cmdList);

#ifdef PIX
	PIXBeginEvent(cmdList, 0, L"Transparent Rendering");
#endif

	// ���� ��ü���� G���ۿ� ���� �� �����Ƿ� 
	// ������ ���������� ���� �׸���.
	cmdList->SetPipelineState(pipelineStateObjects["Transparent"].Get());
	RenderObjects(cmdList, renderableObjects[(int)RenderLayer::Transparent], currentFrameResource->GetObjectVirtualAddress(),&worldCamFrustum);

#ifdef PIX
	PIXEndEvent(cmdList);

	PIXBeginEvent(cmdList, 0, L"Widget Rendering");
#endif

	// ������ 0~6��°������ DebugMap�� �׸��� �����̴�.
	auto iter = widgets.begin();
	std::advance(iter, 7);
	cmdList->SetPipelineState(pipelineStateObjects["Widget"].Get());
	for (; iter != widgets.end(); ++iter)
	{
		// ������ ȭ�鿡 �׸���.
		RenderObject(cmdList, (*iter).get(), currentFrameResource->GetWidgetVirtualAddress());
	}

#ifdef PIX
	PIXEndEvent(cmdList);
#endif

#if defined(DEBUG) || defined(_DEBUG)
	// G���۸� �� �� �ִ� �ؽ�ó ������ �׸���.
	if (GetOptionEnabled(Option::Debug_GBuffer))
	{
		auto iter = widgets.begin();

		cmdList->SetPipelineState(pipelineStateObjects["DiffuseMapDebug"].Get());
		RenderObject(cmdList, (*iter++).get(), currentFrameResource->GetWidgetVirtualAddress());

		cmdList->SetPipelineState(pipelineStateObjects["SpecularMapDebug"].Get());
		RenderObject(cmdList, (*iter++).get(), currentFrameResource->GetWidgetVirtualAddress());

		cmdList->SetPipelineState(pipelineStateObjects["RoughnessMapDebug"].Get());
		RenderObject(cmdList, (*iter++).get(), currentFrameResource->GetWidgetVirtualAddress());

		cmdList->SetPipelineState(pipelineStateObjects["NormalMapDebug"].Get());
		RenderObject(cmdList, (*iter++).get(), currentFrameResource->GetWidgetVirtualAddress());

		cmdList->SetPipelineState(pipelineStateObjects["DepthMapDebug"].Get());
		RenderObject(cmdList, (*iter++).get(), currentFrameResource->GetWidgetVirtualAddress());

#ifdef SSAO
		cmdList->SetPipelineState(pipelineStateObjects["SsaoMapDebug"].Get());
		RenderObject(cmdList, (*iter++).get(), currentFrameResource->GetWidgetVirtualAddress());
#endif

#ifdef SSR
		cmdList->SetPipelineState(pipelineStateObjects["SsrMapDebug"].Get());
		RenderObject(cmdList, (*iter++).get(), currentFrameResource->GetWidgetVirtualAddress());
#endif
	}

	cmdList->SetPipelineState(pipelineStateObjects["Debug"].Get());
	cmdList->SetGraphicsRootSignature(rootSignatures["Debug"].Get());
	cmdList->SetGraphicsRootConstantBufferView((int)RpDebug::Pass, currentFrameResource->GetPassVirtualAddress());
	D3DDebug::GetInstance()->Render(cmdList);
#endif
}

void D3DFramework::PostProcessPass(ID3D12GraphicsCommandList* cmdList)
{
#ifdef SSR
	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvUavDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

#ifdef PIX
	PIXBeginEvent(cmdList, 0, L"Ssr Rendering");
#endif

	// ssr ���� �׸���.
	cmdList->SetGraphicsRootSignature(rootSignatures["Ssr"].Get());
	cmdList->SetPipelineState(pipelineStateObjects["Ssr"].Get());
	ssr->ComputeSsr(cmdList, GetGpuSrv(DescriptorIndex::renderTargetHeapIndex + currentBackBuffer),
		currentFrameResource->GetSsrVirtualAddress(), currentFrameResource->GetPassVirtualAddress());

#ifdef PIX
	PIXEndEvent(cmdList);

	PIXBeginEvent(cmdList, 0, L"Blur");
#endif

	// ssr ���� ����Ų��.
	cmdList->SetComputeRootSignature(rootSignatures["Blur"].Get());
	blurFilter->Execute(cmdList, pipelineStateObjects["BlurHorz"].Get(), pipelineStateObjects["BlurVert"].Get(),
		ssr->Output(), D3D12_RESOURCE_STATE_GENERIC_READ, 1);

#ifdef PIX
	PIXEndEvent(cmdList);

	PIXBeginEvent(cmdList, 0, L"Reflection Rendering");
#endif

	cmdList->RSSetViewports(1, &screenViewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	cmdList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, nullptr);

	cmdList->SetGraphicsRootSignature(rootSignatures["Reflection"].Get());
	cmdList->SetPipelineState(pipelineStateObjects["Reflection"].Get());

	// ssr�� ���Ǵ� ���ҽ����� ���ε��Ѵ�.
	cmdList->SetGraphicsRootDescriptorTable((int)RpReflection::ColorMap, GetGpuSrv(DescriptorIndex::renderTargetHeapIndex + currentBackBuffer));
	cmdList->SetGraphicsRootDescriptorTable((int)RpReflection::BufferMap, GetGpuSrv(DescriptorIndex::deferredBufferHeapIndex + 1));
	cmdList->SetGraphicsRootDescriptorTable((int)RpReflection::SsrMap, GetGpuSrv(DescriptorIndex::ssrMapHeapIndex));

	// ssr���� ����Ͽ� �ݻ�Ǵ� ȿ���� �����Ѵ�.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

#ifdef PIX
	PIXEndEvent(cmdList);
#endif

#endif
}

void D3DFramework::ParticleUpdate(ID3D12GraphicsCommandList* cmdList)
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvUavDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

#ifdef PIX
	PIXBeginEvent(cmdList, 0, L"Particle Update");
#endif
	cmdList->SetComputeRootSignature(rootSignatures["ParticleCompute"].Get());
	cmdList->SetComputeRootConstantBufferView((int)RpParticleCompute::Pass, currentFrameResource->GetPassVirtualAddress());

	cmdList->SetPipelineState(pipelineStateObjects["ParticleEmit"].Get());
	for (const auto& particle : particles)
	{
		if (particle->IsSpawnable())
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = currentFrameResource->GetParticleVirtualAddress() + 
				particle->cbIndex * ConstantsSize::particleCBByteSize;
			cmdList->SetComputeRootConstantBufferView((int)RpParticleCompute::ParticleCB, cbAddress);

			particle->SetBufferUav(cmdList);

			// ��ƼŬ�� �����Ѵ�.
			particle->Emit(cmdList);
		}
	}

	cmdList->SetPipelineState(pipelineStateObjects["ParticleUpdate"].Get());
	for (const auto& particle : particles)
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = currentFrameResource->GetParticleVirtualAddress() +
			particle->cbIndex * ConstantsSize::particleCBByteSize;
		cmdList->SetComputeRootConstantBufferView((int)RpParticleCompute::ParticleCB, cbAddress);

		particle->SetBufferUav(cmdList);

		// ��ƼŬ �����͸� ������Ʈ�Ѵ�.
		particle->Update(cmdList);
	}

	for (const auto& particle : particles)
	{
		// gpu�� ������ ��ƼŬ �����͸� �����Ѵ�.
		particle->CopyData(cmdList);
	}

#ifdef PIX
	PIXEndEvent(cmdList);
#endif
}

void D3DFramework::DrawTerrain(ID3D12GraphicsCommandList* cmdList, bool isWireframe)
{
	cmdList->RSSetViewports(1, &screenViewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvUavDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootSignature(rootSignatures["TerrainRender"].Get());
	if (isWireframe)
	{
#ifdef PIX
		PIXBeginEvent(cmdList, 0, "TerrainWirefrmae Rendering");
#endif

		cmdList->SetPipelineState(pipelineStateObjects["TerrainWireframe"].Get());
	}
	else
	{
#ifdef PIX
		PIXBeginEvent(cmdList, 0, "Terrain Rendering");
#endif

		cmdList->OMSetRenderTargets(DEFERRED_BUFFER_COUNT, &GetDefferedBufferView(0), true, &GetDepthStencilView());
		cmdList->SetPipelineState(pipelineStateObjects["TerrainRender"].Get());
	}
	// ������ ���Ǵ� ���ҽ����� ���ε��Ѵ�.
	cmdList->SetGraphicsRootConstantBufferView((int)RpTerrainGraphics::Pass, currentFrameResource->GetPassVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable((int)RpTerrainGraphics::Texture, GetGpuSrv(DescriptorIndex::textureHeapIndex));
	cmdList->SetGraphicsRootShaderResourceView((int)RpTerrainGraphics::Material, currentFrameResource->GetMaterialVirtualAddress());
	terrain->SetSrvDescriptors(cmdList);

	// ������ �׸���.
	RenderObjects(cmdList, renderableObjects[(int)RenderLayer::Terrain], currentFrameResource->GetTerrainVirtualAddress());

#ifdef PIX
	PIXEndEvent(cmdList);
#endif
}

void D3DFramework::Init(ID3D12GraphicsCommandList* cmdList)
{
#ifdef MULTITHREAD_RENDERING
	for (UINT i = 0; i < FrameResource::processorCoreNum; ++i)
	{
		// ��Ƽ ������ �������� ���Ǵ� ��ɾ� ������Ʈ���� �����Ѵ�.
		auto cmdList = currentFrameResource->worekrCmdLists[i].Get();
		auto cmdAlloc = currentFrameResource->workerCmdAllocs[i].Get();
		Reset(cmdList, cmdAlloc);
	}
#endif

	for (UINT i = 0; i < FrameResource::framePhase; ++i)
	{
		// ���� �����ӿ� ���� ��ɾ� ������Ʈ���� �����Ѵ�.
		auto cmdList = currentFrameResource->frameCmdLists[i].Get();
		auto cmdAlloc = currentFrameResource->frameCmdAllocs[i].Get();
		Reset(cmdList, cmdAlloc);
	}

	// �� G-Buffer�� ���۵�� ���� Ÿ��, ���� ���۸� ����ϱ� ���� �ڿ� ���¸��� ��ȯ�Ѵ�.
	CD3DX12_RESOURCE_BARRIER transitions[DEFERRED_BUFFER_COUNT];
	for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
		transitions[i] = CD3DX12_RESOURCE_BARRIER::Transition(deferredBuffer[i].Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->ResourceBarrier(DEFERRED_BUFFER_COUNT, transitions);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ���� �����ӿ� ���� ���� Ÿ�� �� ���� ���۸� �����.
	for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
		cmdList->ClearRenderTargetView(GetDefferedBufferView(i), (float*)&deferredBufferClearColors[i], 0, nullptr);
	cmdList->ClearRenderTargetView(GetCurrentBackBufferView(), (float*)&backBufferClearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void D3DFramework::MidFrame(ID3D12GraphicsCommandList* cmdList)
{
	// �� G-Buffer�� ���۵�� ���� ���۸� �б� ���� �ڿ� ���¸��� ��ȯ�Ѵ�.
	CD3DX12_RESOURCE_BARRIER transitions[DEFERRED_BUFFER_COUNT];
	for (int i = 0; i < DEFERRED_BUFFER_COUNT; i++)
		transitions[i] = CD3DX12_RESOURCE_BARRIER::Transition(deferredBuffer[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(DEFERRED_BUFFER_COUNT, transitions);
}

void D3DFramework::Finish(ID3D12GraphicsCommandList* cmdList)
{
	// �ĸ� ���۸� ������ �� �ֵ��� ���ҽ� ���¸� �ٲ۴�.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void D3DFramework::WorkerThread(const UINT32 threadIndex)
{
	const UINT32 threadNum = FrameResource::processorCoreNum;

	while (true)
	{
		// G���۸� ä�� �������� ������ ������ ��ٸ���.
		WaitForSingleObject(workerBeginFrameEvents[threadIndex], INFINITE);

		ID3D12GraphicsCommandList* cmdList = nullptr;
#ifdef MULTITHREAD_RENDERING
		// ��Ƽ ������ ���������� ����� ��ɾ� ����Ʈ��
		// ������ �ε�����  ���� �����´�.
		cmdList = currentFrameResource->worekrCmdLists[threadIndex].Get();
#else
		// �̱� ������ �������� �ϳ��� ��ɾ� ����Ʈ�� ���ȴ�.
		cmdList = currentFrameResource->frameCmdLists[0].Get();
#endif

		SetDefaultState(cmdList);
		// ���� G���۸� ��� ���� �ܰ迡 ���ε��Ѵ�.
		cmdList->OMSetRenderTargets(DEFERRED_BUFFER_COUNT, &GetDefferedBufferView(0), true, &GetDepthStencilView());

#ifdef PIX
		PIXBeginEvent(cmdList, 0, L"MultiThread Rendering");
#endif

		cmdList->SetPipelineState(pipelineStateObjects["Opaque"].Get());
		RenderObjects(cmdList, renderableObjects[(int)RenderLayer::Opaque], currentFrameResource->GetObjectVirtualAddress(),
			&worldCamFrustum, threadIndex, threadNum);

		cmdList->SetPipelineState(pipelineStateObjects["AlphaTested"].Get());
		RenderObjects(cmdList, renderableObjects[(int)RenderLayer::AlphaTested], currentFrameResource->GetObjectVirtualAddress(),
			&worldCamFrustum, threadIndex, threadNum);

		cmdList->SetPipelineState(pipelineStateObjects["Billborad"].Get());
		RenderObjects(cmdList, renderableObjects[(int)RenderLayer::Billborad], currentFrameResource->GetObjectVirtualAddress(),
			&worldCamFrustum, threadIndex, threadNum);

#ifdef PIX
		PIXEndEvent(cmdList);
#endif

#ifdef MULTITHREAD_RENDERING
		ThrowIfFailed(cmdList->Close());
#endif

		// �̺�Ʈ�� ��ȣ���·� �ٲپ� ���� ���°� ����ǵ��� �Ѵ�.
		SetEvent(workerFinishedFrameEvents[threadIndex]);
	}
}

void D3DFramework::CreateThreads()
{
	UINT threadNum = FrameResource::processorCoreNum;

	workerThread.reserve(threadNum);
	workerBeginFrameEvents.reserve(threadNum);
	workerFinishedFrameEvents.reserve(threadNum);

	for (UINT i = 0; i < threadNum; ++i)
	{
		workerBeginFrameEvents.push_back(CreateEvent(NULL, FALSE, FALSE, NULL));
		workerFinishedFrameEvents.push_back(CreateEvent(NULL, FALSE, FALSE, NULL));

		workerThread.emplace_back([this, i]() { this->WorkerThread(i); });
	}
}

void D3DFramework::DrawDebugOctree()
{
	octreeRoot->DrawDebug();
}

void D3DFramework::DrawDebugCollision()
{
	for (const auto& obj : gameObjects)
	{
		auto meshBounding = obj->GetCollisionBounding();
		switch (obj->GetCollisionType())
		{
		case CollisionType::AABB:
		{
			const BoundingBox& aabb = std::any_cast<BoundingBox>(meshBounding);
			D3DDebug::GetInstance()->Draw(aabb, FLT_MAX);
			break;
		}
		case CollisionType::OBB:
		{
			const BoundingOrientedBox& obb = std::any_cast<BoundingOrientedBox>(meshBounding);
			D3DDebug::GetInstance()->Draw(obb, FLT_MAX);
			break;
		}
		case CollisionType::Sphere:
		{
			const BoundingSphere& sphere = std::any_cast<BoundingSphere>(meshBounding);
			D3DDebug::GetInstance()->Draw(sphere, FLT_MAX);
			break;
		}
		}
	}
}

void D3DFramework::DrawDebugLight()
{
	static const float lightRadius = 1.0f;

	for (const auto& light : lights)
	{
		D3DDebug::GetInstance()->DrawSphere(light->GetPosition(), lightRadius, FLT_MAX, (XMFLOAT4)Colors::Yellow);
		D3DDebug::GetInstance()->DrawLine(light->GetPosition(), 
			Vector3::Add(light->GetPosition(), Vector3::Multiply(light->GetLook(), 5.0f)), FLT_MAX, (XMFLOAT4)Colors::Yellow);
	}
}