#include "pch.h"
#include "MeshGeometry.h"
#include "D3DUtil.h"
#include "Enums.h"

using namespace DirectX;

MeshGeometry::MeshGeometry(std::string&& name) : Component(std::move(name)) { }

MeshGeometry::~MeshGeometry() { }

void MeshGeometry::BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	void* vertices, std::uint16_t* indices, UINT vertexCount, UINT indexCount, UINT vertexStride, UINT indexStride)
{
	BuildVertices(device, commandList, vertices, vertexCount, vertexStride);
	BuildIndices(device, commandList, indices, indexCount, indexStride);
}

void MeshGeometry::BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	void* vertices, UINT vertexCount, UINT vertexStride)
{
	const UINT vbByteSize = vertexCount * vertexStride;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mVertexBufferCPU));
	CopyMemory(mVertexBufferCPU->GetBufferPointer(), vertices, vbByteSize);

	mVertexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList,
		vertices, vbByteSize, mVertexBufferUploader);

	mVertexBufferView.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = vertexStride;
	mVertexBufferView.SizeInBytes = vbByteSize;
	mVertexCount = vertexCount;
}

void MeshGeometry::BuildIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	std::uint16_t* indices, UINT indexCount, UINT indexStride)
{
	const UINT ibByteSize = indexCount * indexStride;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mIndexBufferCPU));
	CopyMemory(mIndexBufferCPU->GetBufferPointer(), indices, ibByteSize);

	mIndexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList,
		indices, ibByteSize, mIndexBufferUploader);

	mIndexBufferView.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
	mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	mIndexBufferView.SizeInBytes = ibByteSize;
	mIndexCount = indexCount;
}

void MeshGeometry::BuildCollisionBound(XMFLOAT3* points, size_t vertexCount, size_t stride, CollisionType type)
{
	// 충돌 타입에 따라 충돌 Bound를 생성한다.
	mCollisionType = type;
	switch (mCollisionType)
	{
	case CollisionType::AABB:
	{
		BoundingBox aabb;
		BoundingBox::CreateFromPoints(aabb, vertexCount, points, stride);
		mCollisionBounding = std::move(aabb);
		break;
	}
	case CollisionType::OBB:
	{
		BoundingOrientedBox obb;
		BoundingOrientedBox::CreateFromPoints(obb, vertexCount, points, stride);
		mCollisionBounding = std::move(obb);
		break;
	}
	case CollisionType::Sphere:
	{
		BoundingSphere sphere;
		BoundingSphere::CreateFromPoints(sphere, vertexCount, points, stride);
		mCollisionBounding = std::move(sphere);
		break;
	}
	}
}

void MeshGeometry::Render(ID3D12GraphicsCommandList* commandList, UINT instanceCount, bool isIndexed) const
{
	commandList->IASetPrimitiveTopology(mPrimitiveType);
	commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);

	if (isIndexed)
	{
		if (mVertexCount == 0 || mIndexCount == 0)
		{
#if defined(DEBUG) || defined(_DEBUG)
			std::cout << ToString() << " MeshGeometry wasn't Initialize" << std::endl;
#endif
			return;
		}


		commandList->IASetIndexBuffer(&mIndexBufferView);
		commandList->DrawIndexedInstanced(mIndexCount, instanceCount, 0, 0, 0);
	}
	else
	{
		if (mVertexCount == 0)
		{
#if defined(DEBUG) || defined(_DEBUG)
			std::cout << ToString() << " MeshGeometry wasn't Initialize" << std::endl;
#endif
			return;
		}


		commandList->DrawInstanced(mVertexCount, instanceCount, 0, 0);
	}
}

void MeshGeometry::DisposeUploaders()
{
	mVertexBufferUploader = nullptr;
	mIndexBufferUploader = nullptr;
}

void MeshGeometry::SetCollisionBoundingAsAABB(DirectX::XMFLOAT3 extents)
{
	mCollisionType = CollisionType::AABB;

	BoundingBox aabb;
	aabb.Extents = extents;
	mCollisionBounding = std::move(aabb);
}

void MeshGeometry::SetCollisionBoundingAsOBB(DirectX::XMFLOAT3 extents)
{
	mCollisionType = CollisionType::OBB;

	BoundingOrientedBox obb;
	obb.Extents = extents;
	mCollisionBounding = std::move(obb);
}

void MeshGeometry::SetCollisionBoundingAsSphere(float radius)
{
	mCollisionType = CollisionType::Sphere;

	BoundingSphere sphere;
	sphere.Radius = radius;
	mCollisionBounding = std::move(sphere);
}