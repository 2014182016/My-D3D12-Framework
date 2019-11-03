#include "pch.h"
#include "D3DFramework.h"
#include "FrameResource.h"
#include "UploadBuffer.h"
#include "GameTimer.h"
#include "Camera.h"
#include "D3DUtil.h"

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

	// 초기화 명령들을 준비하기 위해 명령 목록들을 재설정한다.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCamera->SetPosition(0.0f, 2.0f, -15.0f);

	objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildBBGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	// 초기화 명령들을 실행한다.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 초기화가 끝날 때까지 기다린다.
	FlushCommandQueue();

	return true;
}

void D3DFramework::OnDestroy()
{
	__super::OnDestroy();

	delete mCamera;
	mCamera = 0;

}

void D3DFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT + 1; // msaa를 위한 추가 렌더 타겟;
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
	OnProcessInput(deltaTime);

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

	AnimateMaterials(deltaTime);
	UpdateObjectCBs(deltaTime);
	UpdateMaterialBuffer(deltaTime);
	UpdateMainPassCB(deltaTime);
}

void D3DFramework::Render()
{
	auto cmdListAlloc = mCurrentFrameResource->mCmdListAlloc;

	// 명령 기록에 관련된 메모리의 재활용을 위해 명령 할당자를 재설정한다.
	// 재설정은 GPU가 관련 명령목록들을 모두 처리한 후에 일어난다.
	ThrowIfFailed(cmdListAlloc->Reset());

	// 명령 목록을 ExcuteCommandList을 통해서 명령 대기열에 추가했다면
	// 명령 목록을 재설정할 수 있다. 명령 목록을 재설정하면 메모리가 재활용된다.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), nullptr));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 후면 버퍼과 깊이 버퍼를 지운다. 
	mCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), (float*)&mBackBufferClearColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 그리기 위한 후면 버퍼과 깊이 버퍼를 지정한다.
	mCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), true, &GetDepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvSrvUavDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// 이 장면에 사용되는 Pass 상수 버퍼를 묶는다.
	auto passCB = mCurrentFrameResource->mPassCB->GetResource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	// 이 장면에서 사영되는 모든 머터리얼을 묶는다. 구조적 버퍼는
	// 힙을 생략하고 그냥 하나의 루트 서술자로 묶을 수 있다.
	auto matBuffer = mCurrentFrameResource->mMaterialBuffer->GetResource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	// 이 장면에 사용되는 모든 텍스처를 묶는다. 테이블의 첫 서술자만 묶으면
	// 테이블에 몇 개의 서술자가 있는지는 Root Signature에 설정되어 있다.
	mCommandList->SetGraphicsRootDescriptorTable(3, mCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	if (mIsWireframe)
	{
		mCommandList->SetPipelineState(mPSOs["Wireframe"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Wireframe]);
	}
	else
	{
		mCommandList->SetPipelineState(mPSOs["Opaque"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

		mCommandList->SetPipelineState(mPSOs["AlphaTested"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

		mCommandList->SetPipelineState(mPSOs["Billborad"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Billborad]);

		mCommandList->SetPipelineState(mPSOs["Transparent"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);
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

void D3DFramework::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	// 마우스를 보이지 않게 한다.
	::SetCursor(NULL);

	SetCapture(mhMainWnd);
}

void D3DFramework::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void D3DFramework::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(mCameraRotateSpeed * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(mCameraRotateSpeed * static_cast<float>(y - mLastMousePos.y));

		mCamera->RotateY(dx);
		mCamera->Pitch(dy);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void D3DFramework::OnProcessInput(float deltaTime)
{
	if (mKeys['w'] || mKeys['W'])
		mCamera->Walk(mCameraWalkSpeed * deltaTime);

	if (mKeys['s'] || mKeys['S'])
		mCamera->Walk(-mCameraWalkSpeed * deltaTime);

	if (mKeys['a'] || mKeys['A'])
		mCamera->Strafe(-mCameraWalkSpeed * deltaTime);

	if (mKeys['d'] || mKeys['D'])
		mCamera->Strafe(mCameraWalkSpeed * deltaTime);

	mCamera->UpdateViewMatrix();
}

void D3DFramework::AnimateMaterials(float deltaTime) { }

void D3DFramework::UpdateObjectCBs(float deltaTime)
{
	auto currObjectCB = mCurrentFrameResource->mObjectCB.get();
	for (auto& e : mAllRitems)
	{
		if (e->mNumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->mWorld);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->mTexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.mWorld, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.mTexTransform, XMMatrixTranspose(texTransform));
			objConstants.mMaterialIndex = e->mMat->mMatCBIndex;

			currObjectCB->CopyData(e->mObjCBIndex, objConstants);

			e->mNumFramesDirty--;
		}
	}
}

void D3DFramework::UpdateMaterialBuffer(float deltaTime)
{
	auto currMaterialBuffer = mCurrentFrameResource->mMaterialBuffer.get();
	for (auto& e : mMaterials)
	{
		Material* mat = e.second.get();
		if (mat->mNumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->mMatTransform);

			MaterialData matData;
			matData.mDiffuseAlbedo = mat->mDiffuseAlbedo;
			matData.mFresnelR0 = mat->mFresnelR0;
			matData.mRoughness = mat->mRoughness;
			XMStoreFloat4x4(&matData.mMatTransform, XMMatrixTranspose(matTransform));
			matData.mDiffuseMapIndex = mat->mDiffuseSrvHeapIndex;
			matData.mNormalMapIndex = mat->mNormalSrvHeapIndex;

			currMaterialBuffer->CopyData(mat->mMatCBIndex, matData);

			mat->mNumFramesDirty--;
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

void D3DFramework::LoadTextures()
{
	// 추가할 텍스쳐 이름과 파일의 이름을 함께 저장
	std::vector<std::pair<std::string, std::wstring>> texNames =
	{
		std::make_pair<std::string, std::wstring>("BricksDiffuseMap", L"Textures/Bricks2.dds"),
		std::make_pair<std::string, std::wstring>("BricksNormalMap", L"Textures/Bricks2_nmap.dds"),
		std::make_pair<std::string, std::wstring>("TileDiffuseMap", L"Textures/Tile.dds"),
		std::make_pair<std::string, std::wstring>("TileNormalMap", L"Textures/Tile_nmap.dds"),
		std::make_pair<std::string, std::wstring>("IceDiffuseMap", L"Textures/Ice.dds"),
		std::make_pair<std::string, std::wstring>("DefaultNormalMap", L"Textures/Default_nmap.dds"),
		std::make_pair<std::string, std::wstring>("FenceDuffuseMap", L"Textures/WireFence.dds"),
		std::make_pair<std::string, std::wstring>("Tree0", L"Textures/Tree01S.dds"),
	};

	mTextureNum = (UINT)texNames.size();

	for (UINT i = 0; i < mTextureNum; ++i)
	{
		auto texMap = std::make_unique<Texture>();
		texMap->mName = texNames[i].first;
		texMap->mFilename = texNames[i].second;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
			mCommandList.Get(), texMap->mFilename.c_str(),
			texMap->mResource, texMap->mUploadHeap));

		mTextures[texMap->mName] = std::move(texMap);
	}
}

void D3DFramework::BuildRootSignature()
{
	// 일반적으로 셰이더 프로그램은 특정 자원들(상수 버퍼, 텍스처, 표본추출기 등)이
	// 입력된다고 기대한다. 루트 서명은 셰이더 프로그램이 기대하는 자원들을 정의한다.

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mTextureNum, 0, 0);

	// 퍼포먼스 TIP: 가장 자주 사용하는 것을 앞에 놓는다.
	slotRootParameter[0].InitAsConstantBufferView(0); // Object 상수 버퍼
	slotRootParameter[1].InitAsConstantBufferView(1); // Pass 상수 버퍼
	slotRootParameter[2].InitAsShaderResourceView(0, 1); // Materials
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // Textures

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Root Signature를 직렬화한 후 객체를 생성한다.
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
	cbvSrvUavDescriptorHeap.NumDescriptors = mTextureNum;
	cbvSrvUavDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvUavDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvSrvUavDescriptorHeap, IID_PPV_ARGS(&mCbvSrvUavDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	std::vector<ComPtr<ID3D12Resource>> tex2DList =
	{
		mTextures["BricksDiffuseMap"]->mResource,
		mTextures["BricksNormalMap"]->mResource,
		mTextures["TileDiffuseMap"]->mResource,
		mTextures["TileNormalMap"]->mResource,
		mTextures["IceDiffuseMap"]->mResource,
		mTextures["DefaultNormalMap"]->mResource,
		mTextures["FenceDuffuseMap"]->mResource,
		mTextures["Tree0"]->mResource
	};

	// 텍스처의 Shader Resource View를 설명하는 구조체
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// 각 텍스처마다 힙에 Shader Resource View를 생성한다.
	for (UINT i = 0; i < (UINT)tex2DList.size(); ++i)
	{
		srvDesc.Format = tex2DList[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(tex2DList[i].Get(), &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
}

void D3DFramework::BuildShadersAndInputLayout()
{
	// Direct3D에게 각 정점의 성분을 알려주는 InputLayout과
	// 셰이더를 컴파일한다.

	// 셰이더에서 사용할 define을 설정한다.
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

void D3DFramework::BuildShapeGeometry()
{
	std::vector<GeometryGenerator::MeshData> meshDatas(5);
	std::vector<SubmeshGeometry> submeshGeomtries;

	GeometryGenerator::CreateBox(meshDatas[0], 1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::CreateGrid(meshDatas[1], 20.0f, 30.0f, 60, 40);
	GeometryGenerator::CreateSphere(meshDatas[2], 0.5f, 20, 20);
	GeometryGenerator::CreateCylinder(meshDatas[3], 0.5f, 0.3f, 3.0f, 20, 20);
	GeometryGenerator::CreateQuad(meshDatas[4], 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);

	UINT vertexOffset = 0, indexOffset = 0, totlaVertexCount = 0, totalIndexCount = 0;
	for (auto& meshData : meshDatas)
	{
		SubmeshGeometry submesh;
		submesh.mIndexCount = (UINT)meshData.Indices32.size();
		submesh.mStartIndexLocation = indexOffset;
		submesh.mBaseVertexLocation = vertexOffset;
		submeshGeomtries.push_back(submesh);

		vertexOffset += (UINT)meshData.Vertices.size();
		indexOffset += (UINT)meshData.Indices32.size();
	}

	totlaVertexCount = vertexOffset;
	totalIndexCount = indexOffset;

	std::vector<Vertex> vertices(totlaVertexCount);
	std::vector<std::uint16_t> indices;

	UINT k = 0, j = 0;
	for (auto& meshData : meshDatas)
	{
		for (size_t i = 0; i < meshData.Vertices.size(); ++i, ++k)
		{
			vertices[k].mPos = meshData.Vertices[i].Position;
			vertices[k].mNormal = meshData.Vertices[i].Normal;
			vertices[k].mTexC = meshData.Vertices[i].TexC;
			vertices[k].mTangentU = meshData.Vertices[i].TangentU;
		}

		indices.insert(indices.end(), std::begin(meshData.GetIndices16()), std::end(meshData.GetIndices16()));
	}

	const UINT vbByteSize = totlaVertexCount * sizeof(Vertex);
	const UINT ibByteSize = totalIndexCount * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->mName = "BasicShapes";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->mVertexBufferCPU));
	CopyMemory(geo->mVertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->mIndexBufferCPU));
	CopyMemory(geo->mIndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->mVertexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->mVertexBufferUploader);

	geo->mIndexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->mIndexBufferUploader);

	geo->mVertexByteStride = sizeof(Vertex);
	geo->mVertexBufferByteSize = vbByteSize;
	geo->mIndexFormat = DXGI_FORMAT_R16_UINT;
	geo->mIndexBufferByteSize = ibByteSize;

	geo->mDrawArgs["Box"] = submeshGeomtries[0];
	geo->mDrawArgs["Grid"] = submeshGeomtries[1];
	geo->mDrawArgs["Sphere"] = submeshGeomtries[2];
	geo->mDrawArgs["Cylinder"] = submeshGeomtries[3];
	geo->mDrawArgs["Quad"] = submeshGeomtries[4];

	mGeometries[geo->mName] = std::move(geo);
}

void D3DFramework::BuildBBGeometry()
{
	struct BillboardVertex
	{
		XMFLOAT3 mPos;
		XMFLOAT2 mSize;
	};

	static const int treeCount = 1;
	std::array<BillboardVertex, 1> vertices;
	for (UINT i = 0; i < treeCount; ++i)
	{
		float x = 0.0f;
		float z = 0.0f;
		float y = 5.0f;

		vertices[i].mPos = XMFLOAT3(x, y, z);
		vertices[i].mSize = XMFLOAT2(5.0f, 5.0f);
	}

	std::array<std::uint16_t, 1> indices = { 0 };

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(vertices);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->mName = "Billboard";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->mVertexBufferCPU));
	CopyMemory(geo->mVertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->mIndexBufferCPU));
	CopyMemory(geo->mIndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->mVertexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->mVertexBufferUploader);

	geo->mIndexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->mIndexBufferUploader);

	geo->mVertexByteStride = sizeof(BillboardVertex);
	geo->mVertexBufferByteSize = vbByteSize;
	geo->mIndexFormat = DXGI_FORMAT_R16_UINT;
	geo->mIndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.mIndexCount = (UINT)indices.size();
	submesh.mStartIndexLocation = 0;
	submesh.mBaseVertexLocation = 0;

	geo->mDrawArgs["Point"] = submesh;

	mGeometries["Billboard"] = std::move(geo);
}

void D3DFramework::BuildPSOs()
{
	// Direct3D에서 사용할 파이프라인의 상태는 PSO객체에 의해 미리 지정된다.
	// 이 모든 것을 하나의 집합체로 지정하는 이유는 Direct3D가 모든 상태가
	// 호환되는지 미리 검증할 수 있으며 드라이버는 하드웨어 상태의 프로그래밍을
	// 위한 모든 코드를 미리 생성할 수 있기 때문이다.


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
	// 불투명한 물체끼리 색상이 누적되어 보이게 하려면 깊이 쓰기를 비활성화하거나 불투명한 물체를 정렬하여 그려야 한다.
	// D3D12_DEPTH_WRITE_MASK_ZERO는 깊이 기록만 비활성화할 뿐 깊이 읽기와 깊이 판정은 여전히 활성화한다.
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
	// MSAA가 활성화되어 있다면 하드웨어는 알파 값을 이용해서 MSAA에 알파 값을 포괄하여 샘플링한다.
	billboardPsoDesc.BlendState.AlphaToCoverageEnable = m4xMsaaState;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&billboardPsoDesc, IID_PPV_ARGS(&mPSOs["Billborad"])));

}

void D3DFramework::BuildFrameResources()
{
	for (int i = 0; i < NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources[i] = std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size());
	}
}

void D3DFramework::BuildMaterials()
{
	UINT matCBIndex = 0;

	auto bricks0 = std::make_unique<Material>();
	bricks0->mName = "Bricks0";
	bricks0->mMatCBIndex = matCBIndex++;
	bricks0->mDiffuseSrvHeapIndex = 0;
	bricks0->mNormalSrvHeapIndex = 1;
	bricks0->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks0->mFresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	bricks0->mRoughness = 0.25f;

	auto tile0 = std::make_unique<Material>();
	tile0->mName = "Tile0";
	tile0->mMatCBIndex = matCBIndex++;
	tile0->mDiffuseSrvHeapIndex = 2;
	tile0->mNormalSrvHeapIndex = 3;
	tile0->mDiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tile0->mFresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	tile0->mRoughness = 0.1f;

	auto mirror0 = std::make_unique<Material>();
	mirror0->mName = "Mirror0";
	mirror0->mMatCBIndex = matCBIndex++;
	mirror0->mDiffuseSrvHeapIndex = 4;
	mirror0->mNormalSrvHeapIndex = 5;
	mirror0->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	mirror0->mFresnelR0 = XMFLOAT3(0.08f, 0.08f, 0.08f);
	mirror0->mRoughness = 0.1f;

	auto wirefence = std::make_unique<Material>();
	wirefence->mName = "Wirefence";
	wirefence->mMatCBIndex = matCBIndex++;
	wirefence->mDiffuseSrvHeapIndex = 6;
	wirefence->mNormalSrvHeapIndex = 5;
	wirefence->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence->mFresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	wirefence->mRoughness = 0.25f;

	auto treeSprites = std::make_unique<Material>();
	treeSprites->mName = "Tree0";
	treeSprites->mMatCBIndex = matCBIndex++;
	treeSprites->mDiffuseSrvHeapIndex = 7;
	treeSprites->mNormalSrvHeapIndex = 5;
	treeSprites->mDiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->mFresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->mRoughness = 0.125f;


	mMaterials["Bricks0"] = std::move(bricks0);
	mMaterials["Tile0"] = std::move(tile0);
	mMaterials["Mirror0"] = std::move(mirror0);
	mMaterials["Wirefence"] = std::move(wirefence);
	mMaterials["Tree0"] = std::move(treeSprites);
}

void D3DFramework::BuildRenderItems()
{
	UINT objCBIndex = 0;

	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->mWorld, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 1.1f, 0.0f));
	XMStoreFloat4x4(&boxRitem->mTexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	boxRitem->mObjCBIndex = objCBIndex++;
	boxRitem->mMat = mMaterials["Wirefence"].get();
	boxRitem->mGeo = mGeometries["BasicShapes"].get();
	boxRitem->mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->mSubmesh = boxRitem->mGeo->mDrawArgs["Box"];
	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(boxRitem.get());
	mRitemLayer[(int)RenderLayer::Wireframe].push_back(boxRitem.get());
	mAllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->mWorld = D3DUtil::Identity4x4f();
	XMStoreFloat4x4(&gridRitem->mTexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->mObjCBIndex = objCBIndex++;
	gridRitem->mMat = mMaterials["Tile0"].get();
	gridRitem->mGeo = mGeometries["BasicShapes"].get();
	gridRitem->mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->mSubmesh = gridRitem->mGeo->mDrawArgs["Grid"];
	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	mRitemLayer[(int)RenderLayer::Wireframe].push_back(gridRitem.get());
	mAllRitems.push_back(std::move(gridRitem));

	XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		auto rightCylRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylRitem->mWorld, rightCylWorld);
		XMStoreFloat4x4(&leftCylRitem->mTexTransform, brickTexTransform);
		leftCylRitem->mObjCBIndex = objCBIndex++;
		leftCylRitem->mMat = mMaterials["Bricks0"].get();
		leftCylRitem->mGeo = mGeometries["BasicShapes"].get();
		leftCylRitem->mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->mSubmesh = leftCylRitem->mGeo->mDrawArgs["Cylinder"];

		XMStoreFloat4x4(&rightCylRitem->mWorld, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->mTexTransform, brickTexTransform);
		rightCylRitem->mObjCBIndex = objCBIndex++;
		rightCylRitem->mMat = mMaterials["Bricks0"].get();
		rightCylRitem->mGeo = mGeometries["BasicShapes"].get();
		rightCylRitem->mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->mSubmesh = rightCylRitem->mGeo->mDrawArgs["Cylinder"];

		XMStoreFloat4x4(&leftSphereRitem->mWorld, leftSphereWorld);
		leftSphereRitem->mTexTransform = D3DUtil::Identity4x4f();
		leftSphereRitem->mObjCBIndex = objCBIndex++;
		leftSphereRitem->mMat = mMaterials["Mirror0"].get();
		leftSphereRitem->mGeo = mGeometries["BasicShapes"].get();
		leftSphereRitem->mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->mSubmesh = leftSphereRitem->mGeo->mDrawArgs["Sphere"];

		XMStoreFloat4x4(&rightSphereRitem->mWorld, rightSphereWorld);
		rightSphereRitem->mTexTransform = D3DUtil::Identity4x4f();
		rightSphereRitem->mObjCBIndex = objCBIndex++;
		rightSphereRitem->mMat = mMaterials["Mirror0"].get();
		rightSphereRitem->mGeo = mGeometries["BasicShapes"].get();
		rightSphereRitem->mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->mSubmesh = rightSphereRitem->mGeo->mDrawArgs["Sphere"];

		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
		mRitemLayer[(int)RenderLayer::Transparent].push_back(leftSphereRitem.get());
		mRitemLayer[(int)RenderLayer::Transparent].push_back(rightSphereRitem.get());
		mRitemLayer[(int)RenderLayer::Wireframe].push_back(leftCylRitem.get());
		mRitemLayer[(int)RenderLayer::Wireframe].push_back(rightCylRitem.get());
		mRitemLayer[(int)RenderLayer::Wireframe].push_back(leftSphereRitem.get());
		mRitemLayer[(int)RenderLayer::Wireframe].push_back(rightSphereRitem.get());

		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}

	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->mWorld = D3DUtil::Identity4x4f();
	treeSpritesRitem->mObjCBIndex = objCBIndex++;
	treeSpritesRitem->mMat = mMaterials["Tree0"].get();
	treeSpritesRitem->mGeo = mGeometries["Billboard"].get();
	treeSpritesRitem->mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->mSubmesh = treeSpritesRitem->mGeo->mDrawArgs["Point"];
	mRitemLayer[(int)RenderLayer::Billborad].push_back(treeSpritesRitem.get());
	mAllRitems.push_back(std::move(treeSpritesRitem));
}

void D3DFramework::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	auto objectCB = mCurrentFrameResource->mObjectCB->GetResource();
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->mGeo->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&ri->mGeo->GetIndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->mPrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->mObjCBIndex*objCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->mSubmesh.mIndexCount, 1, ri->mSubmesh.mStartIndexLocation, ri->mSubmesh.mBaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> D3DFramework::GetStaticSamplers()
{
	// 그래픽 응용 프로그램이 사용하는 표본추출기의 수는 그리 많지 않으므로
	// 미리 만들어서 Root Signature에 포함시켜 둔다.

	// 점 필터링, 순환 좌표 지정 모드
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	// 점 필터링, 한정 좌표 지정 모드
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	// 선형 필터링, 순환 좌표 지정 모드
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	// 선형 필터링, 한정 좌표 지정 모드
	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	// 비등방 필터링, 순환 좌표 지정 모드
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy
	
	// 비등방 필터링, 한정 좌표 지정 모드
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

void D3DFramework::OnKeyDown(unsigned int input)
{
	__super::OnKeyDown(input);

}

void D3DFramework::OnKeyUp(unsigned int input)
{
	__super::OnKeyUp(input);

	if (input == VK_F1)
		mIsWireframe = !mIsWireframe;
	else if (input == VK_F4)
		SetFullscreenState(!mFullscreenState);
	
}