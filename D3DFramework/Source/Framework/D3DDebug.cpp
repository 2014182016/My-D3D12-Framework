#include "../PrecompiledHeader/pch.h"
#include "D3DDebug.h"
#include "D3DStructure.h"
#include "../Component/Mesh.h"

using namespace std::literals;

D3DDebug::D3DDebug() { }

D3DDebug::~D3DDebug() 
{
	meshBuildCmdList = nullptr;
	meshBuildCmdAlloc = nullptr;
	cmdQueue = nullptr;
	fence = nullptr;

	debugDatas.clear();
}

D3DDebug* D3DDebug::GetInstance()
{
	static D3DDebug* instance = nullptr;
	if (instance == nullptr)
		instance = new D3DDebug();
	return instance;
}


void D3DDebug::CreateCommandObjects(ID3D12Device* device)
{
	d3dDevice = device;

	// ��ɾ� ť�� �����Ѵ�.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));

	// ��ɾ� �Ҵ��ڸ� �����Ѵ�.
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(meshBuildCmdAlloc.GetAddressOf())));

	// ��ɾ� ����Ʈ�� �����Ѵ�.
	ThrowIfFailed(d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		meshBuildCmdAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(meshBuildCmdList.GetAddressOf())));

	// �潺�� �����Ѵ�.
	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void D3DDebug::DrawLine(const XMFLOAT3& p1, const XMFLOAT3& p2, const float time, const XMFLOAT4& color)
{
	DebugData data;
	std::vector<LineVertex> vertices(2);

	data.debugMesh = new Mesh("DebugLine"s);

	vertices.emplace_back(p1, color);
	vertices.emplace_back(p2, color);

	BuildDebugMesh(data, vertices, time);
}

void D3DDebug::DrawRay(const XMVECTOR& p1, const XMVECTOR& p2, const float time, const XMFLOAT4& color)
{
	DrawLine(Vector3::XMVectorToFloat3(p1), Vector3::XMVectorToFloat3(p2), time, color);
}

void D3DDebug::DrawBox(const XMFLOAT3& center, const XMFLOAT3& extents, const float time, const XMFLOAT4& color)
{
	DebugData data;
	std::vector<LineVertex> vertices(24);

	data.debugMesh = new Mesh("DebugBox"s);

	XMFLOAT3 p[] = 
	{ 
		XMFLOAT3(center.x + extents.x, center.y - extents.y, center.z - extents.z),
		XMFLOAT3(center.x + extents.x, center.y - extents.y, center.z + extents.z),
		XMFLOAT3(center.x - extents.x, center.y - extents.y, center.z + extents.z),
		XMFLOAT3(center.x - extents.x, center.y - extents.y, center.z - extents.z),
		XMFLOAT3(center.x + extents.x, center.y + extents.y, center.z - extents.z),
		XMFLOAT3(center.x + extents.x, center.y + extents.y, center.z + extents.z),
		XMFLOAT3(center.x - extents.x, center.y + extents.y, center.z + extents.z),
		XMFLOAT3(center.x - extents.x, center.y + extents.y, center.z - extents.z)
	};

	vertices.emplace_back(p[0], color);	vertices.emplace_back(p[1], color);
	vertices.emplace_back(p[1], color);	vertices.emplace_back(p[2], color);
	vertices.emplace_back(p[2], color);	vertices.emplace_back(p[3], color);
	vertices.emplace_back(p[3], color);	vertices.emplace_back(p[0], color);
	vertices.emplace_back(p[4], color);	vertices.emplace_back(p[5], color);
	vertices.emplace_back(p[5], color);	vertices.emplace_back(p[6], color);
	vertices.emplace_back(p[6], color);	vertices.emplace_back(p[7], color);
	vertices.emplace_back(p[7], color);	vertices.emplace_back(p[4], color);
	vertices.emplace_back(p[0], color);	vertices.emplace_back(p[4], color);
	vertices.emplace_back(p[1], color);	vertices.emplace_back(p[5], color);
	vertices.emplace_back(p[2], color);	vertices.emplace_back(p[6], color);
	vertices.emplace_back(p[3], color);	vertices.emplace_back(p[7], color);

	BuildDebugMesh(data, vertices, time);
}

void D3DDebug::DrawSphere(const XMFLOAT3& center, const float radius, const float time, const XMFLOAT4& color)
{
	XMVECTOR xmvCenter = XMLoadFloat3(&center);

	XMVECTOR xAxis = g_XMIdentityR0 * radius;
	XMVECTOR yAxis = g_XMIdentityR1 * radius;
	XMVECTOR zAxis = g_XMIdentityR2 * radius;

	DrawRing(xmvCenter, xAxis, zAxis, time, color);
	DrawRing(xmvCenter, xAxis, yAxis, time, color);
	DrawRing(xmvCenter, yAxis, zAxis, time, color);
}

void D3DDebug::DrawRing(const XMVECTOR& center, const XMVECTOR& majorAxis, const XMVECTOR& minorAxis,
	const float time, const XMFLOAT4& color)
{
	static const int ringSegments = 32;
	static constexpr float angleDelta = XM_2PI / float(ringSegments);

	DebugData data;
	std::vector<LineVertex> vertices(ringSegments + 1);

	data.debugMesh = new Mesh("DebugRing"s);

	// �������� ����Ѵ�.
	XMVECTOR cosDelta = XMVectorReplicate(cosf(angleDelta));
	XMVECTOR sinDelta = XMVectorReplicate(sinf(angleDelta));
	XMVECTOR incrementalSin = XMVectorReplicate(0.0f);
	XMVECTOR incrementalCos = XMVectorReplicate(1.0f);

	for (int i = 0; i < ringSegments + 1; ++i)
	{
		// ���ο� ��ġ�� ����Ѵ�.
		XMVECTOR pos = XMVectorMultiplyAdd(majorAxis, incrementalCos, center);
		pos = XMVectorMultiplyAdd(minorAxis, incrementalSin, pos);

		// ��ġ�� ���� �����Ѵ�.
		XMStoreFloat3(&vertices[i].pos, pos);
		vertices[i].color = color;

		// ���ο� �������� ����Ѵ�.
		XMVECTOR newCos = incrementalCos * cosDelta - incrementalSin * sinDelta;
		XMVECTOR newSin = incrementalCos * sinDelta + incrementalSin * cosDelta;
		incrementalCos = newCos;
		incrementalSin = newSin;
	}

	BuildDebugMeshWithTopology(data, vertices, time, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
}

void D3DDebug::Draw(const BoundingFrustum& frustum, const float time, const XMFLOAT4& color)
{
	DebugData data;
	std::vector<LineVertex> vertices(24);

	data.debugMesh = new Mesh("DebugFrstum"s);

	XMFLOAT3 corners[BoundingFrustum::CORNER_COUNT];
	frustum.GetCorners(corners);

	vertices[0].pos = corners[0];
	vertices[1].pos = corners[1];
	vertices[2].pos = corners[1];
	vertices[3].pos = corners[2];
	vertices[4].pos = corners[2];
	vertices[5].pos = corners[3];
	vertices[6].pos = corners[3];
	vertices[7].pos = corners[0];

	vertices[8].pos = corners[0];
	vertices[9].pos = corners[4];
	vertices[10].pos = corners[1];
	vertices[11].pos = corners[5];
	vertices[12].pos = corners[2];
	vertices[13].pos = corners[6];
	vertices[14].pos = corners[3];
	vertices[15].pos = corners[7];

	vertices[16].pos = corners[4];
	vertices[17].pos = corners[5];
	vertices[18].pos = corners[5];
	vertices[19].pos = corners[6];
	vertices[20].pos = corners[6];
	vertices[21].pos = corners[7];
	vertices[22].pos = corners[7];
	vertices[23].pos = corners[4];

	for (size_t i = 0; i < vertices.size(); ++i)
	{
		vertices[i].color = color;
	}

	BuildDebugMesh(data, vertices, time);
}

void D3DDebug::Draw(const DirectX::BoundingOrientedBox& obb, const float time, const XMFLOAT4& color)
{
	DebugData data;
	std::vector<LineVertex> vertices(24);

	data.debugMesh = new Mesh("DebugBox"s);

	XMFLOAT3 corners[BoundingOrientedBox::CORNER_COUNT];
	obb.GetCorners(corners);

	vertices.emplace_back(corners[0], color);	vertices.emplace_back(corners[1], color);
	vertices.emplace_back(corners[1], color);	vertices.emplace_back(corners[2], color);
	vertices.emplace_back(corners[2], color);	vertices.emplace_back(corners[3], color);
	vertices.emplace_back(corners[3], color);	vertices.emplace_back(corners[0], color);
	vertices.emplace_back(corners[4], color);	vertices.emplace_back(corners[5], color);
	vertices.emplace_back(corners[5], color);	vertices.emplace_back(corners[6], color);
	vertices.emplace_back(corners[6], color);	vertices.emplace_back(corners[7], color);
	vertices.emplace_back(corners[7], color);	vertices.emplace_back(corners[4], color);
	vertices.emplace_back(corners[0], color);	vertices.emplace_back(corners[4], color);
	vertices.emplace_back(corners[1], color);	vertices.emplace_back(corners[5], color);
	vertices.emplace_back(corners[2], color);	vertices.emplace_back(corners[6], color);
	vertices.emplace_back(corners[3], color);	vertices.emplace_back(corners[7], color);

	BuildDebugMesh(data, vertices, time);
}

void D3DDebug::Draw(const DirectX::BoundingBox& aabb, const float time, const XMFLOAT4& color)
{
	DrawBox(aabb.Center, aabb.Extents, time, color);
}

void D3DDebug::Draw(const DirectX::BoundingSphere& sphere, const float time, const XMFLOAT4& color)
{
	DrawSphere(sphere.Center, sphere.Radius, time, color);
}

void D3DDebug::Update(float deltaTime)
{
	if (isMeshBuild)
	{
		// �ش� ��ɾ ����� ������ ��ٸ���.
		FlushCommandQueue();
		isMeshBuild = false;

		// ���� ��ɾ ���ؼ� �����Ѵ�.
		Reset(meshBuildCmdList.Get(), meshBuildCmdAlloc.Get());
	}

	if (!debugDatas.empty())
	{
		for (auto iter = debugDatas.begin(); iter != debugDatas.end();)
		{
			if (iter->time < 0.0f)
			{
				// ���� �ð��� 0���ϱ� �Ǹ� �������� ���δ�.
				// ���� �����ӿ��� �ش� ��ü�� �׸��µ��� ����ϰ� �����Ƿ�
				// �ش� ��ü�� �׸��� �ʴ� GPU Ÿ�Ӷ��ο� �����ؾ� �Ѵ�.
				--iter->frame;

				// ������ �� �Ǹ� �ش� ��ü�� ����Ʈ���� �����Ѵ�.
				if (iter->frame == 0)
				{
					delete iter->debugMesh;
					iter->debugMesh = nullptr;
					iter = debugDatas.erase(iter);
					continue;
				}
			}

			iter->time -= deltaTime;
			++iter;
		}
	}
}

void D3DDebug::Render(ID3D12GraphicsCommandList* cmdList)
{
	for (const auto& data : debugDatas)
	{
		// ����� �޽��� ������ 0�̻��� �� �׸� �� �ִ�.
		// frame�� 3�� �ƴϸ� ������ 0���Ϸ� frame ���� ���ҵǾ��� �� �̴�.
		if (data.frame == NUM_FRAME_RESOURCES)
		{
			data.debugMesh->Render(cmdList, 1, false);
		}
	}
}

void D3DDebug::Clear()
{
	for (auto& data : debugDatas)
	{
		// ������ ���ָ� ���� ������Ʈ����
		// ��� �����͸� �����ϰ� �ȴ�.
		data.time = 0.0f;
	}
}

void D3DDebug::ExcuteBuild()
{
	// ����� �޽��� �����ϴ� ��ɾ �����Ѵ�.
	ThrowIfFailed(meshBuildCmdList->Close());
	ID3D12CommandList* cmdLists[] = { meshBuildCmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}


void D3DDebug::Reset(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc)
{
	// ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� �缳���Ѵ�.
	// �缳���� GPU�� ���� ��ɸ�ϵ��� ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(cmdAlloc->Reset());

	// ��� ����� ExcuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ�
	// ��� ����� �缳���� �� �ִ�. ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(cmdList->Reset(cmdAlloc, nullptr));
}

void D3DDebug::FlushCommandQueue()
{
	ExcuteBuild();

	// ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϵ��� ��Ÿ�� ���� ������Ų��.
	currentFence++;

	// �� ��Ÿ�� ������ �����ϴ� ���(Signal)�� ��� ��⿭�� �߰��Ѵ�.
	// �� ��Ÿ�� ������ GPU�� Signal() ��ɱ����� ��� ����� ó���ϱ�
	// �������� �������� �ʴ´�.
	ThrowIfFailed(cmdQueue->Signal(fence.Get(), currentFence));

	// GPU�� �� ��Ÿ�� ���������� ��ɵ��� �Ϸ��� ������ ��ٸ���.
	if (fence->GetCompletedValue() < currentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// GPU�� ���� ��Ÿ�� ������ ���������� �̺�Ʈ�� �ߵ��Ѵ�.
		ThrowIfFailed(fence->SetEventOnCompletion(currentFence, eventHandle));

		// GPU�� ���� ��Ÿ�� ������ ���������� ���ϴ� �̺�Ʈ�� ��ٸ���.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void D3DDebug::BuildDebugMeshWithTopology(DebugData& data, const std::vector<LineVertex>& vertices, const float time,
	D3D_PRIMITIVE_TOPOLOGY topology)
{
	// ���� ���۸� �����Ѵ�.
	data.debugMesh->BuildVertices(d3dDevice, meshBuildCmdList.Get(),
	(void*)vertices.data(), (UINT32)vertices.size(), (UINT32)sizeof(LineVertex));

	// ������Ƽ��� �ð��� �����Ѵ�.
	data.debugMesh->SetPrimitiveType(topology);
	data.time = time;

	// ����� �޽��� �����Ѵ�.
	debugDatas.push_back(data);

	isMeshBuild = true;
}

void D3DDebug::BuildDebugMesh(DebugData& data, const std::vector<LineVertex>& vertices, const float time)
{
	BuildDebugMeshWithTopology(data, vertices, time, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}