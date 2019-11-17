#include "pch.h"
#include "MeshGeometry.h"
#include "D3DUtil.h"
#include "Enums.h"

using namespace DirectX;

MeshGeometry::MeshGeometry(std::string name) : Base(name) { }

MeshGeometry::~MeshGeometry() { }

void MeshGeometry::BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	void* vertices, std::uint16_t* indices, UINT vertexCount, UINT indexCount, UINT vertexStride, UINT indexStride)
{
	// 버텍스 버퍼와 인덱스 버퍼를 업로드 버퍼에 저장한다.
	const UINT vbByteSize = vertexCount * vertexStride;
	const UINT ibByteSize = indexCount * indexStride;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mVertexBufferCPU));
	CopyMemory(mVertexBufferCPU->GetBufferPointer(), vertices, vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mIndexBufferCPU));
	CopyMemory(mIndexBufferCPU->GetBufferPointer(), indices, ibByteSize);

	mVertexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList,
		vertices, vbByteSize, mVertexBufferUploader);

	mIndexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList,
		indices, ibByteSize, mIndexBufferUploader);

	mVertexBufferView.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = vertexStride;
	mVertexBufferView.SizeInBytes = vbByteSize;

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

void MeshGeometry::Render(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetPrimitiveTopology(mPrimitiveType);
	commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	commandList->IASetIndexBuffer(&mIndexBufferView);

	commandList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
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