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

	// 명령어 큐를 생성한다.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));

	// 명령어 할당자를 생성한다.
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(meshBuildCmdAlloc.GetAddressOf())));

	// 명령어 리스트를 생성한다.
	ThrowIfFailed(d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		meshBuildCmdAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(meshBuildCmdList.GetAddressOf())));

	// 펜스를 생성한다.
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

	// 증가량을 계산한다.
	XMVECTOR cosDelta = XMVectorReplicate(cosf(angleDelta));
	XMVECTOR sinDelta = XMVectorReplicate(sinf(angleDelta));
	XMVECTOR incrementalSin = XMVectorReplicate(0.0f);
	XMVECTOR incrementalCos = XMVectorReplicate(1.0f);

	for (int i = 0; i < ringSegments + 1; ++i)
	{
		// 새로운 위치를 계산한다.
		XMVECTOR pos = XMVectorMultiplyAdd(majorAxis, incrementalCos, center);
		pos = XMVectorMultiplyAdd(minorAxis, incrementalSin, pos);

		// 위치와 색을 저장한다.
		XMStoreFloat3(&vertices[i].pos, pos);
		vertices[i].color = color;

		// 새로운 증가량을 계산한다.
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
		// 해당 명령어가 수행될 때까지 기다린다.
		FlushCommandQueue();
		isMeshBuild = false;

		// 다음 명령어를 위해서 리셋한다.
		Reset(meshBuildCmdList.Get(), meshBuildCmdAlloc.Get());
	}

	if (!debugDatas.empty())
	{
		for (auto iter = debugDatas.begin(); iter != debugDatas.end();)
		{
			if (iter->time < 0.0f)
			{
				// 수명 시간이 0이하기 되면 프레임을 줄인다.
				// 이전 프레임에서 해당 객체를 그리는데에 사용하고 있으므로
				// 해당 객체를 그리지 않는 GPU 타임라인에 도달해야 한다.
				--iter->frame;

				// 수명이 다 되면 해당 객체를 리스트에서 삭제한다.
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
		// 디버그 메쉬는 수명이 0이상일 때 그릴 수 있다.
		// frame이 3이 아니면 수명이 0이하로 frame 값이 감소되었을 때 이다.
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
		// 수명을 없애면 다음 업데이트에서
		// 모든 데이터를 삭제하게 된다.
		data.time = 0.0f;
	}
}

void D3DDebug::ExcuteBuild()
{
	// 디버그 메쉬를 생성하는 명령어를 수행한다.
	ThrowIfFailed(meshBuildCmdList->Close());
	ID3D12CommandList* cmdLists[] = { meshBuildCmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}


void D3DDebug::Reset(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc)
{
	// 명령 기록에 관련된 메모리의 재활용을 위해 명령 할당자를 재설정한다.
	// 재설정은 GPU가 관련 명령목록들을 모두 처리한 후에 일어난다.
	ThrowIfFailed(cmdAlloc->Reset());

	// 명령 목록을 ExcuteCommandList을 통해서 명령 대기열에 추가했다면
	// 명령 목록을 재설정할 수 있다. 명령 목록을 재설정하면 메모리가 재활용된다.
	ThrowIfFailed(cmdList->Reset(cmdAlloc, nullptr));
}

void D3DDebug::FlushCommandQueue()
{
	ExcuteBuild();

	// 현재 울타리 지점까지의 명령들을 표시하도록 울타리 값을 전진시킨다.
	currentFence++;

	// 이 울타리 지점을 설정하는 명령(Signal)을 명령 대기열에 추가한다.
	// 새 울타리 지점은 GPU가 Signal() 명령까지의 모든 명령을 처리하기
	// 전까지는 설정되지 않는다.
	ThrowIfFailed(cmdQueue->Signal(fence.Get(), currentFence));

	// GPU가 이 울타리 지점까지의 명령들을 완료할 때까지 기다린다.
	if (fence->GetCompletedValue() < currentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// GPU가 현재 울타리 지점에 도달했으면 이벤트를 발동한다.
		ThrowIfFailed(fence->SetEventOnCompletion(currentFence, eventHandle));

		// GPU가 현재 울타리 지점에 도달했음을 뜻하는 이벤트를 기다린다.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void D3DDebug::BuildDebugMeshWithTopology(DebugData& data, const std::vector<LineVertex>& vertices, const float time,
	D3D_PRIMITIVE_TOPOLOGY topology)
{
	// 정점 버퍼를 생성한다.
	data.debugMesh->BuildVertices(d3dDevice, meshBuildCmdList.Get(),
	(void*)vertices.data(), (UINT32)vertices.size(), (UINT32)sizeof(LineVertex));

	// 프리미티브와 시간을 설정한다.
	data.debugMesh->SetPrimitiveType(topology);
	data.time = time;

	// 디버그 메쉬를 저장한다.
	debugDatas.push_back(data);

	isMeshBuild = true;
}

void D3DDebug::BuildDebugMesh(DebugData& data, const std::vector<LineVertex>& vertices, const float time)
{
	BuildDebugMeshWithTopology(data, vertices, time, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}