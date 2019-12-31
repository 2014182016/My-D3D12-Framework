#include "pch.h"
#include "D3DDebug.h"
#include "Structures.h"
#include "MeshGeometry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

D3DDebug* D3DDebug::GetInstance()
{
	if (instance == nullptr)
		instance = new D3DDebug();
	return instance;
}

D3DDebug::D3DDebug() { }

D3DDebug::~D3DDebug()  { }

void D3DDebug::Initialize(ID3D12Device* device)
{
	if (isInitialized)
		return;

	md3dDevice = device;

	CreateDirectObjects();

	CommandListReset();

	CreateDebugMeshes();

	CommandListExcute();
	FlushCommandQueue();

	isInitialized = true;
}

void D3DDebug::CreateDebugMeshes()
{
	std::unique_ptr<MeshGeometry> mesh;
	std::vector<DebugVertex> vertices;
	std::vector<std::uint16_t> indices;
	GeometryGenerator::MeshData meshData;

	meshData = GeometryGenerator::CreateBox(2.0f, 2.0f, 2.0f, 1);
	mesh = std::make_unique<MeshGeometry>("Debug_CollisionBox");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position);
	mesh->BuildVertices(md3dDevice, mCommandList.Get(), (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(DebugVertex));
	mesh->BuildIndices(md3dDevice, mCommandList.Get(), meshData.GetIndices16().data(), (UINT)meshData.GetIndices16().size(), (UINT)sizeof(std::uint16_t));
	mDebugMeshes[(int)DebugType::Debug_CollisionBox] = std::move(mesh);
	vertices.clear();
	indices.clear();

	meshData = GeometryGenerator::CreateSphere(1.0f, 10, 10);
	mesh = std::make_unique<MeshGeometry>("Debug_CollisionSphere");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position);
	mesh->BuildVertices(md3dDevice, mCommandList.Get(), (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(DebugVertex));
	mesh->BuildIndices(md3dDevice, mCommandList.Get(), meshData.GetIndices16().data(), (UINT)meshData.GetIndices16().size(), (UINT)sizeof(std::uint16_t));
	mDebugMeshes[(int)DebugType::Debug_CollisionSphere] = std::move(mesh);
	vertices.clear();
	indices.clear();

	meshData = GeometryGenerator::CreateBox(1.0f, 1.0f, 2.0f, 1);
	mesh = std::make_unique<MeshGeometry>("Debug_Light");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position);
	mesh->BuildVertices(md3dDevice, mCommandList.Get(), (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(DebugVertex));
	mesh->BuildIndices(md3dDevice, mCommandList.Get(), meshData.GetIndices16().data(), (UINT)meshData.GetIndices16().size(), (UINT)sizeof(std::uint16_t));
	mDebugMeshes[(int)DebugType::Debug_Light] = std::move(mesh);
	vertices.clear();
	indices.clear();

	mesh = std::make_unique<MeshGeometry>("Debug_OctTree");
	vertices.emplace_back(XMFLOAT3(1.0f, 1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-1.0f, 1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-1.0f, 1.0f, -1.0f));
	vertices.emplace_back(XMFLOAT3(1.0f, 1.0f, -1.0f));
	vertices.emplace_back(XMFLOAT3(1.0f, -1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-1.0f, -1.0f, 1.0f));
	vertices.emplace_back(XMFLOAT3(-1.0f, -1.0f, -1.0f));
	vertices.emplace_back(XMFLOAT3(1.0f, -1.0f, -1.0f));
	indices.emplace_back(0); indices.emplace_back(1);
	indices.emplace_back(1); indices.emplace_back(2);
	indices.emplace_back(2); indices.emplace_back(3);
	indices.emplace_back(3); indices.emplace_back(0);
	indices.emplace_back(4); indices.emplace_back(5);
	indices.emplace_back(5); indices.emplace_back(6);
	indices.emplace_back(6); indices.emplace_back(7);
	indices.emplace_back(7); indices.emplace_back(4);
	indices.emplace_back(0); indices.emplace_back(4);
	indices.emplace_back(1); indices.emplace_back(5);
	indices.emplace_back(2); indices.emplace_back(6);
	indices.emplace_back(3); indices.emplace_back(7);
	mesh->BuildVertices(md3dDevice, mCommandList.Get(), (void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(DebugVertex));
	mesh->BuildIndices(md3dDevice, mCommandList.Get(), indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));
	mesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	mDebugMeshes[(int)DebugType::Debug_Octree] = std::move(mesh);
	vertices.clear();
}

void D3DDebug::Reset()
{
	for (auto& vec : mDebugConstants)
		vec.clear();
}

UINT D3DDebug::GetDebugDataCount() const
{
	UINT dataCount = 0;

	for (const auto& vec : mDebugConstants)
	{
		dataCount += (UINT)vec.size();
	}

	return dataCount;
}

void D3DDebug::CreateDirectObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	// 닫힌 상태로 시작한다. 이후 명령 목록을 처음 참조할 때 Reset을 호출하는데,
	// Reset을 호출하려면 CommandList가 닫혀 있어야 하기 때문이다.
	mCommandList->Close();
}

void D3DDebug::FlushCommandQueue()
{
	// 현재 울타리 지점까지의 명령들을 표시하도록 울타리 값을 전진시킨다.
	mCurrentFence++;

	// 이 울타리 지점을 설정하는 명령(Signal)을 명령 대기열에 추가한다.
	// 새 울타리 지점은 GPU가 Signal() 명령까지의 모든 명령을 처리하기
	// 전까지는 설정되지 않는다.
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// GPU가 이 울타리 지점까지의 명령들을 완료할 때까지 기다린다.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// GPU가 현재 울타리 지점에 도달했으면 이벤트를 발동한다.
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// GPU가 현재 울타리 지점에 도달했음을 뜻하는 이벤트를 기다린다.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void D3DDebug::CommandListReset()
{
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
}

void D3DDebug::CommandListExcute()
{
	// 초기화 명령들을 실행한다.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void D3DDebug::DrawLine(XMFLOAT3 p1, XMFLOAT3 p2, float time, XMFLOAT4 color)
{
	std::vector<LineVertex> vertices;
	std::unique_ptr<MeshGeometry> line = std::make_unique<MeshGeometry>("Line");

	CommandListReset();

	vertices.emplace_back(p1, color);
	vertices.emplace_back(p2, color);

	line->BuildVertices(md3dDevice, mCommandList.Get(), vertices.data(), (UINT)vertices.size(), (UINT)sizeof(LineVertex));
	line->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	auto linePair = std::make_pair(time, std::move(line));
	mLines.push_front(std::move(linePair));

	CommandListExcute();
	FlushCommandQueue();
}

void D3DDebug::DrawLine(XMVECTOR p1, XMVECTOR p2, float time, XMFLOAT4 color)
{
	XMFLOAT3 p1f, p2f;
	XMStoreFloat3(&p1f, p1);
	XMStoreFloat3(&p2f, p2);
	DrawLine(p1f, p2f, time, color);
}

void D3DDebug::DrawCube(XMFLOAT3 center, float width, float height, float depth, float time, XMFLOAT4 color)
{
	std::vector<LineVertex> vertices;
	std::unique_ptr<MeshGeometry> cube = std::make_unique<MeshGeometry>("Cube");

	CommandListReset();

	XMFLOAT3 p0 = XMFLOAT3(center.x + width, center.y + height, center.z + depth);
	XMFLOAT3 p1 = XMFLOAT3(center.x - width, center.y + height, center.z + depth);
	XMFLOAT3 p2 = XMFLOAT3(center.x - width, center.y + height, center.z - depth);
	XMFLOAT3 p3 = XMFLOAT3(center.x + width, center.y + height, center.z - depth);
	XMFLOAT3 p4 = XMFLOAT3(center.x + width, center.y - height, center.z + depth);
	XMFLOAT3 p5 = XMFLOAT3(center.x - width, center.y - height, center.z + depth);
	XMFLOAT3 p6 = XMFLOAT3(center.x - width, center.y - height, center.z - depth);
	XMFLOAT3 p7 = XMFLOAT3(center.x + width, center.y - height, center.z - depth);

	vertices.emplace_back(p0, color); vertices.emplace_back(p1, color);
	vertices.emplace_back(p1, color); vertices.emplace_back(p2, color);
	vertices.emplace_back(p2, color); vertices.emplace_back(p3, color);
	vertices.emplace_back(p3, color); vertices.emplace_back(p0, color);
	vertices.emplace_back(p4, color); vertices.emplace_back(p5, color);
	vertices.emplace_back(p5, color); vertices.emplace_back(p6, color);
	vertices.emplace_back(p6, color); vertices.emplace_back(p7, color);
	vertices.emplace_back(p7, color); vertices.emplace_back(p4, color);
	vertices.emplace_back(p0, color); vertices.emplace_back(p4, color);
	vertices.emplace_back(p1, color); vertices.emplace_back(p5, color);
	vertices.emplace_back(p2, color); vertices.emplace_back(p6, color);
	vertices.emplace_back(p3, color); vertices.emplace_back(p7, color);

	cube->BuildVertices(md3dDevice, mCommandList.Get(), vertices.data(), (UINT)vertices.size(), (UINT)sizeof(LineVertex));
	cube->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	auto cubePair = std::make_pair(time, std::move(cube));
	mLines.push_front(std::move(cubePair));

	CommandListExcute();
	FlushCommandQueue();
}

void D3DDebug::DrawCube(XMVECTOR center, float width, float height, float depth, float time, XMFLOAT4 color)
{
	XMFLOAT3 centerf;
	XMStoreFloat3(&centerf, center);
	DrawCube(centerf, width, height, depth, time, color);
}

void D3DDebug::DrawCube(XMFLOAT3 center, XMFLOAT3 extent, float time, XMFLOAT4 color)
{
	DrawCube(center, extent.x, extent.y, extent.z, time, color);
}

void D3DDebug::DrawCube(XMVECTOR center, XMVECTOR extent, float time, XMFLOAT4 color)
{
	XMFLOAT3 centerf, extentf;
	XMStoreFloat3(&centerf, center);
	XMStoreFloat3(&extentf, extent);
	DrawCube(centerf, extentf.x, extentf.y, extentf.z, time, color);
}

void D3DDebug::DrawCube(XMFLOAT3 center, float width, float time, XMFLOAT4 color)
{
	DrawCube(center, width, width, width, time, color);
}

void D3DDebug::DrawCube(XMVECTOR center, float width, float time, XMFLOAT4 color)
{
	XMFLOAT3 centerf;
	XMStoreFloat3(&centerf, center);
	DrawCube(centerf, width, width, width, time, color);
}

void D3DDebug::DrawRing(DirectX::XMFLOAT3 center, XMVECTOR majorAxis, XMVECTOR minorAxis, float raidus, float time, DirectX::XMFLOAT4 color)
{
	static const UINT segments = 16;
	std::vector<LineVertex> vertices;
	std::unique_ptr<MeshGeometry> ring = std::make_unique<MeshGeometry>("Ring");
	
	CommandListReset();

	float angleDelta = XM_2PI / float(segments);
	XMVECTOR origin = XMLoadFloat3(&center);
	XMVECTOR cosDelta = XMVectorReplicate(cosf(angleDelta));
	XMVECTOR sinDelta = XMVectorReplicate(sinf(angleDelta));
	XMVECTOR incrementSin = XMVectorZero();
	XMVECTOR incrementCos = XMVectorReplicate(1.0f);

	XMFLOAT3 lastPos;
	XMVECTOR pos = XMVectorMultiplyAdd(majorAxis, incrementCos * raidus, origin);
	pos = XMVectorMultiplyAdd(minorAxis, incrementSin * raidus, pos);
	XMStoreFloat3(&lastPos, pos);

	XMVECTOR newCos = incrementCos * cosDelta - incrementSin * sinDelta;
	XMVECTOR newSin = incrementCos * sinDelta + incrementSin * cosDelta;
	incrementCos = newCos;
	incrementSin = newSin;

	for (UINT i = 0; i < segments; ++i)
	{
		XMFLOAT3 currPos;

		XMVECTOR pos = XMVectorMultiplyAdd(majorAxis, incrementCos * raidus, origin);
		pos = XMVectorMultiplyAdd(minorAxis, incrementSin * raidus, pos);
		XMStoreFloat3(&currPos, pos);

		vertices.emplace_back(lastPos, color);
		vertices.emplace_back(currPos, color);

		XMVECTOR newCos = incrementCos * cosDelta - incrementSin * sinDelta;
		XMVECTOR newSin = incrementCos * sinDelta + incrementSin * cosDelta;
		incrementCos = newCos;
		incrementSin = newSin;

		lastPos = currPos;
	}

	ring->BuildVertices(md3dDevice, mCommandList.Get(), vertices.data(), (UINT)vertices.size(), (UINT)sizeof(LineVertex));
	ring->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	auto ringPair = std::make_pair(time, std::move(ring));
	mLines.push_front(std::move(ringPair));

	CommandListExcute();
	FlushCommandQueue();
}

void D3DDebug::DrawSphere(XMFLOAT3 center, float radius, float time, XMFLOAT4 color)
{
	XMVECTOR xaxis = g_XMIdentityR0;
	XMVECTOR yaxis = g_XMIdentityR1;
	XMVECTOR zaxis = g_XMIdentityR2;

	DrawRing(center, xaxis, zaxis, radius, time, color);
	DrawRing(center, xaxis, yaxis, radius, time, color);
	DrawRing(center, yaxis, zaxis, radius, time, color);
}

void D3DDebug::DrawSphere(XMVECTOR center, float radius, float time, XMFLOAT4 color)
{
	XMFLOAT3 centerf;
	XMStoreFloat3(&centerf, center);
	DrawSphere(centerf, radius, time, color);
}

void D3DDebug::RenderLine(ID3D12GraphicsCommandList* cmdList, float deltaTime)
{
	if (mLines.empty())
		return;

	// 시간이 모두 지나더라도 3프레임이전의 line은 CommandList가 계속해서 그리고 잇는 중이다.
	// 따라서 3프레임이 지나면 line 객체를 삭제하도록 한다.
	mLines.remove_if([](const std::pair<float, std::unique_ptr<class MeshGeometry>>& line)->bool
	{ return !line.second->IsUpdate(); });

	for (auto& line : mLines)
	{
		// ms단위로 지난 시간만큼을 뺀다.
		line.first -= deltaTime;
		if (line.first < 0.0f)
			// 시간이 모두 지나면 1프레임 감소시킨다.
			line.second->DecreaseNumFrames();
		else
			// 시간이 남아있다면 line은 그린다.
			line.second->Render(cmdList, 1, false);
	}
}