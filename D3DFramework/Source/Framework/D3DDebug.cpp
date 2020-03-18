#include "pch.h"
#include "D3DDebug.h"
#include "D3DUtil.h"
#include "Structure.h"
#include "Mesh.h"

using namespace DirectX;
using namespace std::literals;

D3DDebug::D3DDebug() { }

D3DDebug::~D3DDebug() 
{
	mMeshBuildCmdList = nullptr;
	mMeshBuildCmdAlloc = nullptr;
	mCmdQueue = nullptr;
	mFence = nullptr;

	mDebugDatas.clear();

	delete this;
}

D3DDebug* D3DDebug::GetInstance()
{
	if (instance == nullptr)
		instance = new D3DDebug();
	return instance;
}


void D3DDebug::CreateCommandObjects(ID3D12Device* device)
{
	mDevice = device;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue)));

	ThrowIfFailed(mDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mMeshBuildCmdAlloc.GetAddressOf())));

	ThrowIfFailed(mDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mMeshBuildCmdAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(mMeshBuildCmdList.GetAddressOf())));

	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

void D3DDebug::DrawLine(const XMFLOAT3& p1, const XMFLOAT3& p2, const float time, const XMFLOAT4& color)
{
	DebugData data;
	std::vector<LineVertex> vertices(2);

	data.mDebugMesh = new Mesh("DebugLine"s);

	vertices.emplace_back(p1, color);
	vertices.emplace_back(p2, color);

	BuildDebugMesh(data, vertices, time);
}

void D3DDebug::DrawRay(const XMVECTOR& p1, const XMVECTOR& p2, const float time, const XMFLOAT4& color)
{
	XMFLOAT3 xmf3P1, xmf3P2;

	XMStoreFloat3(&xmf3P1, p1);
	XMStoreFloat3(&xmf3P2, p2);

	DrawLine(xmf3P1, xmf3P2, time, color);
}

void D3DDebug::DrawBox(const XMFLOAT3& center, const XMFLOAT3& extents, const float time, const XMFLOAT4& color)
{
	DebugData data;
	std::vector<LineVertex> vertices(24);

	data.mDebugMesh = new Mesh("DebugBox"s);

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

	data.mDebugMesh = new Mesh("DebugRing"s);

	XMVECTOR cosDelta = XMVectorReplicate(cosf(angleDelta));
	XMVECTOR sinDelta = XMVectorReplicate(sinf(angleDelta));
	XMVECTOR incrementalSin = XMVectorReplicate(0.0f);
	XMVECTOR incrementalCos = XMVectorReplicate(1.0f);

	for (int i = 0; i < ringSegments + 1; ++i)
	{
		XMVECTOR pos = XMVectorMultiplyAdd(majorAxis, incrementalCos, center);
		pos = XMVectorMultiplyAdd(minorAxis, incrementalSin, pos);

		XMStoreFloat3(&vertices[i].mPosition, pos);
		vertices[i].mColor = color;

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

	data.mDebugMesh = new Mesh("DebugFrstum"s);

	XMFLOAT3 corners[BoundingFrustum::CORNER_COUNT];
	frustum.GetCorners(corners);

	vertices[0].mPosition = corners[0];
	vertices[1].mPosition = corners[1];
	vertices[2].mPosition = corners[1];
	vertices[3].mPosition = corners[2];
	vertices[4].mPosition = corners[2];
	vertices[5].mPosition = corners[3];
	vertices[6].mPosition = corners[3];
	vertices[7].mPosition = corners[0];

	vertices[8].mPosition = corners[0];
	vertices[9].mPosition = corners[4];
	vertices[10].mPosition = corners[1];
	vertices[11].mPosition = corners[5];
	vertices[12].mPosition = corners[2];
	vertices[13].mPosition = corners[6];
	vertices[14].mPosition = corners[3];
	vertices[15].mPosition = corners[7];

	vertices[16].mPosition = corners[4];
	vertices[17].mPosition = corners[5];
	vertices[18].mPosition = corners[5];
	vertices[19].mPosition = corners[6];
	vertices[20].mPosition = corners[6];
	vertices[21].mPosition = corners[7];
	vertices[22].mPosition = corners[7];
	vertices[23].mPosition = corners[4];

	for (size_t i = 0; i < vertices.size(); ++i)
	{
		vertices[i].mColor = color;
	}

	BuildDebugMesh(data, vertices, time);
}

void D3DDebug::Draw(const DirectX::BoundingOrientedBox& obb, const float time, const XMFLOAT4& color)
{
	DebugData data;
	std::vector<LineVertex> vertices(24);

	data.mDebugMesh = new Mesh("DebugBox"s);

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
	if (mIsMeshBuild)
	{
		FlushCommandQueue();
		mIsMeshBuild = false;

		Reset(mMeshBuildCmdList.Get(), mMeshBuildCmdAlloc.Get());
	}

	if (!mDebugDatas.empty())
	{
		for (auto iter = mDebugDatas.begin(); iter != mDebugDatas.end();)
		{
			if (iter->mTime < 0.0f)
			{
				// 수명 시간이 0이하기 되면 프레임을 줄인다.
				// 이전 프레임에서 해당 객체를 그리는데에 사용하고 있으므로
				// 해당 객체를 그리지 않는 GPU 타임라인에 도달해야 한다.
				--iter->mFrame;

				// 수명이 다 되면 해당 객체를 리스트에서 삭제한다.
				if (iter->mFrame == 0)
				{
					delete iter->mDebugMesh;
					iter->mDebugMesh = nullptr;
					iter = mDebugDatas.erase(iter);
					continue;
				}
			}

			iter->mTime -= deltaTime;
			++iter;
		}
	}
}

void D3DDebug::Render(ID3D12GraphicsCommandList* cmdList)
{
	for (const auto& data : mDebugDatas)
	{
		// 디버그 메쉬는 수명이 0이상일 때 그릴 수 있다.
		// frame이 3이 아니면 수명이 0이하로 frame 값이 감소되었을 때 이다.
		if (data.mFrame == NUM_FRAME_RESOURCES)
		{
			data.mDebugMesh->Render(cmdList, 1, false);
		}
	}
}

void D3DDebug::Clear()
{
	for (auto& data : mDebugDatas)
	{
		data.mTime = 0.0f;
	}
}

void D3DDebug::ExcuteBuild()
{
	ThrowIfFailed(mMeshBuildCmdList->Close());
	ID3D12CommandList* cmdLists[] = { mMeshBuildCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
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
	mCurrentFence++;

	// 이 울타리 지점을 설정하는 명령(Signal)을 명령 대기열에 추가한다.
	// 새 울타리 지점은 GPU가 Signal() 명령까지의 모든 명령을 처리하기
	// 전까지는 설정되지 않는다.
	ThrowIfFailed(mCmdQueue->Signal(mFence.Get(), mCurrentFence));

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

void D3DDebug::BuildDebugMeshWithTopology(DebugData& data, const std::vector<LineVertex>& vertices, const float time,
	D3D_PRIMITIVE_TOPOLOGY topology)
{
	data.mDebugMesh->BuildVertices(mDevice, mMeshBuildCmdList.Get(),
		(void*)vertices.data(), (UINT)vertices.size(), (UINT)sizeof(LineVertex));

	data.mDebugMesh->SetPrimitiveType(topology);
	data.mTime = time;

	mDebugDatas.push_back(data);

	mIsMeshBuild = true;
}

void D3DDebug::BuildDebugMesh(DebugData& data, const std::vector<LineVertex>& vertices, const float time)
{
	BuildDebugMeshWithTopology(data, vertices, time, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}