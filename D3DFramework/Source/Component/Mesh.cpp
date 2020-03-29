#include <Component/Mesh.h>
#include <iostream>

Mesh::Mesh(std::string&& name) : Component(std::move(name)) { }

Mesh::~Mesh() { }

void Mesh::BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	void* vertices, const UINT32 vertexCount, const UINT32 vertexStride)
{
	// 복사할 데이터 크기를 계산한다.
	const UINT32 vbByteSize = vertexCount * vertexStride;

	// 정점들을 데이터 크기만큼 복사한다.
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &vertexBufferCPU));
	CopyMemory(vertexBufferCPU->GetBufferPointer(), vertices, vbByteSize);

	// 업로드 버퍼를 통하여 정점 버퍼에 데이터를 복사한다.
	vertexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, vertices, vbByteSize, vertexBufferUploader);

	// 서술자 뷰를 정의한다.
	vertexBufferView.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = vertexStride;
	vertexBufferView.SizeInBytes = vbByteSize;
	this->vertexCount = vertexCount;
}

void Mesh::BuildIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	 UINT16* indices, const UINT32 indexCount, const UINT32 indexStride)
{
	// 복사할 데이터 크기를 계산한다.
	const UINT32 ibByteSize = indexCount * indexStride;

	// 인덱스들을 데이터 크기만큼 복사한다.
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCPU));
	CopyMemory(indexBufferCPU->GetBufferPointer(), indices, ibByteSize);

	// 업로드 버퍼를 통하여 인덱스 버퍼에 데이터를 복사한다.
	indexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList, indices, ibByteSize, indexBufferUploader);

	// 서술자 뷰를 정의한다.
	indexBufferView.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = ibByteSize;
	this->indexCount = indexCount;
}

void Mesh::BuildCollisionBound(XMFLOAT3* points, const UINT32 vertexCount, const UINT32 stride, const CollisionType type)
{
	// 충돌 타입에 따라 충돌 바운드를 생성한다.
	collisionType = type;
	switch (collisionType)
	{
	case CollisionType::AABB:
	{
		BoundingBox aabb;
		BoundingBox::CreateFromPoints(aabb, vertexCount, points, stride);
		collisionBounding = std::move(aabb);
		break;
	}
	case CollisionType::OBB:
	{
		BoundingOrientedBox obb;
		BoundingOrientedBox::CreateFromPoints(obb, vertexCount, points, stride);
		collisionBounding = std::move(obb);
		break;
	}
	case CollisionType::Sphere:
	{
		BoundingSphere sphere;
		BoundingSphere::CreateFromPoints(sphere, vertexCount, points, stride);
		collisionBounding = std::move(sphere);
		break;
	}
	}
}

void Mesh::SetDynamicVertexBuffer(ID3D12Resource* vb, const UINT32 vertexCount, const UINT32 vertexStride)
{
	const UINT32 vbByteSize = vertexCount * vertexStride;

	// 동적 정점 버퍼로서 사용할 서술자 뷰를 정의한다.
	// 실제 정점 버퍼는 프레임 자원에 존재한다.
	vertexBufferGPU = vb;
	vertexBufferView.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = vertexStride;
	vertexBufferView.SizeInBytes = vbByteSize;
	this->vertexCount = vertexCount;
}

void Mesh::Render(ID3D12GraphicsCommandList* commandList, const UINT32 instanceCount, const bool isIndexed) const
{
	commandList->IASetPrimitiveTopology(primitiveType);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	if (isIndexed)
	{
		if (vertexCount == 0 || indexCount == 0)
		{
#if defined(DEBUG) || defined(_DEBUG)
			std::cout << ToString() << " Mesh wasn't Initialize" << std::endl;
#endif
			return;
		}

		commandList->IASetIndexBuffer(&indexBufferView);
		commandList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
	}
	else
	{
		if (vertexCount == 0)
		{
#if defined(DEBUG) || defined(_DEBUG)
			std::cout << ToString() << " Mesh wasn't Initialize" << std::endl;
#endif
			return;
		}

		commandList->DrawInstanced(vertexCount, instanceCount, 0, 0);
	}
}

void Mesh::DisposeUploaders()
{
	// 업로드 버퍼의 메모리를 해제한다.
	vertexBufferUploader = nullptr;
	indexBufferUploader = nullptr;
}

void Mesh::SetCollisionBoundingAsAABB(const XMFLOAT3& extents)
{
	collisionType = CollisionType::AABB;

	BoundingBox aabb;
	aabb.Extents = extents;
	collisionBounding = std::move(aabb);
}

void Mesh::SetCollisionBoundingAsOBB(const XMFLOAT3& extents)
{
	collisionType = CollisionType::OBB;

	BoundingOrientedBox obb;
	obb.Extents = extents;
	collisionBounding = std::move(obb);
}

void Mesh::SetCollisionBoundingAsSphere(const float radius)
{
	collisionType = CollisionType::Sphere;

	BoundingSphere sphere;
	sphere.Radius = radius;
	collisionBounding = std::move(sphere);
}

void Mesh::SetPrimitiveType(const D3D12_PRIMITIVE_TOPOLOGY primitiveType)
{
	this->primitiveType = primitiveType; 
}
CollisionType Mesh::GetCollisionType() const
{
	return collisionType; 
}
std::any Mesh::GetCollisionBounding() const
{
	return collisionBounding; 
}