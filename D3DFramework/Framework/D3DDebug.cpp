#include "pch.h"
#include "D3DDebug.h"
#include "MeshGeometry.h"
#include "GeometryGenerator.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

D3DDebug::D3DDebug()
{
	assert(!mInstance);
	mInstance = this;

	CreateDirectObjects();
	CommandListReset();

	std::unique_ptr<MeshGeometry> mesh;
	std::vector<DebugVertex> vertices;
	std::vector<std::uint16_t> indices;
	GeometryGenerator::MeshData meshData;

	mDebugMeshes.resize((int)DebugType::Count);

	meshData = GeometryGenerator::CreateBox(2.0f, 2.0f, 2.0f, 1);
	mesh = std::make_unique<MeshGeometry>("Debug_CollisionBox");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position, (XMFLOAT4)Colors::Red);
	mesh->BuildMesh(md3dDevice.Get(), mCommandList.Get(), vertices.data(), meshData.GetIndices16().data(),
		(UINT)vertices.size(), (UINT)meshData.GetIndices16().size(), (UINT)sizeof(DebugVertex), (UINT)sizeof(std::uint16_t));
	mDebugMeshes[(int)DebugType::Debug_CollisionBox] = std::move(mesh);
	vertices.clear();
	indices.clear();

	meshData = GeometryGenerator::CreateSphere(1.0f, 5, 5);
	mesh = std::make_unique<MeshGeometry>("Debug_CollisionSphere");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position, (XMFLOAT4)Colors::Red);
	mesh->BuildMesh(md3dDevice.Get(), mCommandList.Get(), vertices.data(), meshData.GetIndices16().data(),
		(UINT)vertices.size(), (UINT)meshData.GetIndices16().size(), (UINT)sizeof(DebugVertex), (UINT)sizeof(std::uint16_t));
	mDebugMeshes[(int)DebugType::Debug_CollisionSphere] = std::move(mesh);
	vertices.clear();
	indices.clear();

	meshData = GeometryGenerator::CreateBox(1.0f, 1.0f, 2.0f, 1);
	mesh = std::make_unique<MeshGeometry>("Debug_Light");
	for (const auto& ver : meshData.Vertices)
		vertices.emplace_back(ver.Position, (XMFLOAT4)Colors::Yellow);
	mesh->BuildMesh(md3dDevice.Get(), mCommandList.Get(), vertices.data(), meshData.GetIndices16().data(),
		(UINT)vertices.size(), (UINT)meshData.GetIndices16().size(), (UINT)sizeof(DebugVertex), (UINT)sizeof(std::uint16_t));
	mDebugMeshes[(int)DebugType::Debug_Light] = std::move(mesh);
	vertices.clear();
	indices.clear();

	mesh = std::make_unique<MeshGeometry>("Debug_OctTree");
	vertices.emplace_back(XMFLOAT3(1.0f, 1.0f, 1.0f), (XMFLOAT4)Colors::Green);
	vertices.emplace_back(XMFLOAT3(-1.0f, 1.0f, 1.0f), (XMFLOAT4)Colors::Green);
	vertices.emplace_back(XMFLOAT3(-1.0f, 1.0f, -1.0f), (XMFLOAT4)Colors::Green);
	vertices.emplace_back(XMFLOAT3(1.0f, 1.0f, -1.0f), (XMFLOAT4)Colors::Green);
	vertices.emplace_back(XMFLOAT3(1.0f, -1.0f, 1.0f), (XMFLOAT4)Colors::Green);
	vertices.emplace_back(XMFLOAT3(-1.0f, -1.0f, 1.0f), (XMFLOAT4)Colors::Green);
	vertices.emplace_back(XMFLOAT3(-1.0f, -1.0f, -1.0f), (XMFLOAT4)Colors::Green);
	vertices.emplace_back(XMFLOAT3(1.0f, -1.0f, -1.0f), (XMFLOAT4)Colors::Green);
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
	mesh->BuildMesh(md3dDevice.Get(), mCommandList.Get(), vertices.data(), indices.data(), (UINT)vertices.size(), (UINT)indices.size(),
		(UINT)sizeof(DebugVertex), (UINT)sizeof(std::uint16_t));
	mesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	mDebugMeshes[(int)DebugType::Debug_OctTree] = std::move(mesh);
	vertices.clear();

	mInstanceCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(InstanceConstants));

	CommandListExcute();
	FlushCommandQueue();
}

D3DDebug::~D3DDebug()  { }

void D3DDebug::Reset()
{
	for (auto& vec : mDebugDatas)
		vec.clear();

	mDebugCBIndex = 0;
}

void D3DDebug::RenderDebug(ID3D12GraphicsCommandList* cmdList, BufferMemoryPool<InstanceConstants>* currInstancePool) const
{
	auto currInstanceBuffer = currInstancePool->GetBuffer();
	UINT instanceLocation = 0;
	for (int i = 0; i < (int)DebugType::Count; ++i)
	{
		UINT dataCount = (UINT)mDebugDatas[i].size();
		if (dataCount == 0)
			continue;

		// �ν��Ͻ� ���� ���۸� ������Ʈ�Ѵ�.
		InstanceConstants instanceConstants;
		instanceConstants.mDebugInstanceIndex = instanceLocation;

		currInstanceBuffer->CopyData(i, instanceConstants);
		auto instanceCBAddressStart = currInstanceBuffer->GetResource()->GetGPUVirtualAddress();
		D3D12_GPU_VIRTUAL_ADDRESS instanceCBAddress = instanceCBAddressStart + i * mInstanceCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(5, instanceCBAddress);

		// �ν��Ͻ����� ������ϱ� ���� �޽ø� �׸���.
		mDebugMeshes[i]->Render(cmdList, dataCount);

		instanceLocation += dataCount;
	}
}

void D3DDebug::Update(ID3D12GraphicsCommandList* cmdList, BufferMemoryPool<DebugData>* currDebugPool, const int index)
{
	std::vector<DebugData>& currDatas = mDebugDatas[index];
	if (currDatas.size() == 0)
		return;

	for (const auto& data : currDatas)
	{
		// ����� �����͸� ���ۿ� ������Ʈ�Ѵ�.
		currDebugPool->GetBuffer()->CopyData(mDebugCBIndex, data);

		++mDebugCBIndex;
	}
}

void D3DDebug::AddDebugData(DebugData& data, const DebugType type)
{
	mDebugDatas[(int)type].push_back(data);
}

void D3DDebug::AddDebugData(DebugData&& data, const DebugType type)
{
	mDebugDatas[(int)type].push_back(std::move(data));
}

void D3DDebug::AddDebugData(DebugData&& data, const CollisionType type)
{
	int index = 0;

	switch (type)
	{
	case CollisionType::AABB:
		index = (int)DebugType::AABB;
		break;
	case CollisionType::OBB:
		index = (int)DebugType::OBB;
		break;
	case CollisionType::Sphere:
		index = (int)DebugType::Sphere;
		break;
	default:
		return;
	}

	mDebugDatas[index].push_back(std::move(data));
}

void D3DDebug::AddDebugData(std::vector<DebugData>& datas, const DebugType type)
{
	mDebugDatas[(int)type].insert(mDebugDatas[(int)type].end(), datas.begin(), datas.end());
}

UINT D3DDebug::GetAllDebugDataCount() const
{
	UINT dataCount = 0;

	for (const auto& vec : mDebugDatas)
	{
		dataCount += (UINT)vec.size();
	}

	return dataCount;
}

void D3DDebug::CreateDirectObjects()
{
	ComPtr<IDXGIFactory4> mdxgiFactory;
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

	// ���� ���·� �����Ѵ�. ���� ��� ����� ó�� ������ �� Reset�� ȣ���ϴµ�,
	// Reset�� ȣ���Ϸ��� CommandList�� ���� �־�� �ϱ� �����̴�.
	mCommandList->Close();
}

void D3DDebug::FlushCommandQueue()
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

void D3DDebug::CommandListReset()
{
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
}

void D3DDebug::CommandListExcute()
{
	// �ʱ�ȭ ��ɵ��� �����Ѵ�.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
}

void D3DDebug::DrawLine(XMFLOAT3 p1, XMFLOAT3 p2, float time, XMFLOAT4 color)
{
	std::vector<DebugVertex> vertices;
	std::unique_ptr<MeshGeometry> line = std::make_unique<MeshGeometry>("Line");

	CommandListReset();

	vertices.emplace_back(p1, color);
	vertices.emplace_back(p2, color);

	line->BuildVertices(md3dDevice.Get(), mCommandList.Get(), vertices.data(), (UINT)vertices.size(), (UINT)sizeof(DebugVertex));
	line->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	auto linePair = std::make_pair(time, std::move(line));
	mLines.push_front(std::move(linePair));

	CommandListExcute();
	FlushCommandQueue();
}

void D3DDebug::DrawCube(XMFLOAT3 center, float width, float height, float depth, float time, XMFLOAT4 color)
{
	std::vector<DebugVertex> vertices;
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

	cube->BuildVertices(md3dDevice.Get(), mCommandList.Get(), vertices.data(), (UINT)vertices.size(), (UINT)sizeof(DebugVertex));
	cube->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	auto cubePair = std::make_pair(time, std::move(cube));
	mLines.push_front(std::move(cubePair));

	CommandListExcute();
	FlushCommandQueue();
}

void D3DDebug::DrawCube(XMFLOAT3 center,XMFLOAT3 extent, float time, XMFLOAT4 color)
{
	DrawCube(center, extent.x, extent.y, extent.z, time, color);
}

void D3DDebug::DrawCube(XMFLOAT3 center, float width, float time, XMFLOAT4 color)
{
	DrawCube(center, width, width, width, time, color);
}

void D3DDebug::DrawRing(DirectX::XMFLOAT3 center, XMVECTOR majorAxis, XMVECTOR minorAxis, float raidus, float time, DirectX::XMFLOAT4 color)
{
	static const UINT segments = 16;
	std::vector<DebugVertex> vertices;
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

	ring->BuildVertices(md3dDevice.Get(), mCommandList.Get(), vertices.data(), (UINT)vertices.size(), (UINT)sizeof(DebugVertex));
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

void D3DDebug::RenderLine(ID3D12GraphicsCommandList* cmdList, float deltaTime)
{
	if (mLines.empty())
		return;

	// �ð��� ��� �������� 3������������ line�� CommandList�� ����ؼ� �׸��� �մ� ���̴�.
	// ���� 3�������� ������ line ��ü�� �����ϵ��� �Ѵ�.
	mLines.remove_if([](const std::pair<float, std::unique_ptr<class MeshGeometry>>& line)->bool
	{ return !line.second->IsUpdate(); });

	for (auto& line : mLines)
	{
		// ms������ ���� �ð���ŭ�� ����.
		line.first -= deltaTime;
		if (line.first < 0.0f)
			// �ð��� ��� ������ 1������ ���ҽ�Ų��.
			line.second->DecreaseNumFrames();
		else
			// �ð��� �����ִٸ� line�� �׸���.
			line.second->Render(cmdList, 1, false);
	}
}