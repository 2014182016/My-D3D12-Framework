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
				// ���� �ð��� 0���ϱ� �Ǹ� �������� ���δ�.
				// ���� �����ӿ��� �ش� ��ü�� �׸��µ��� ����ϰ� �����Ƿ�
				// �ش� ��ü�� �׸��� �ʴ� GPU Ÿ�Ӷ��ο� �����ؾ� �Ѵ�.
				--iter->mFrame;

				// ������ �� �Ǹ� �ش� ��ü�� ����Ʈ���� �����Ѵ�.
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
		// ����� �޽��� ������ 0�̻��� �� �׸� �� �ִ�.
		// frame�� 3�� �ƴϸ� ������ 0���Ϸ� frame ���� ���ҵǾ��� �� �̴�.
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
	mCurrentFence++;

	// �� ��Ÿ�� ������ �����ϴ� ���(Signal)�� ��� ��⿭�� �߰��Ѵ�.
	// �� ��Ÿ�� ������ GPU�� Signal() ��ɱ����� ��� ����� ó���ϱ�
	// �������� �������� �ʴ´�.
	ThrowIfFailed(mCmdQueue->Signal(mFence.Get(), mCurrentFence));

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