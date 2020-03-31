#pragma once

#include "Component.h"
#include <any>

/*
��ü�� ȭ�鿡 �������ϱ� ���� �޽�
���� ���� �� �ε��� ���۸� �����ϰ�, �׸���.
*/
class Mesh : public Component
{
public:
	Mesh(std::string&& name);
	virtual ~Mesh();

public:
	// ���� ���۸� �����Ѵ�.
	void BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, void* vertices, 
		const UINT32 vertexCount, const UINT32 vertexStride);
	// �ε��� ���۸� �����Ѵ�.
	void BuildIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT16* indices,
		const UINT32 indexCount, const UINT32 indexStride);
	// �浹 Ÿ�Կ� ���� �ٿ�带 �����Ѵ�.
	void BuildCollisionBound(XMFLOAT3* posPtr, const UINT32 vertexCount, const UINT32 stride, 
		const CollisionType type);
	// ���� ���� ���۸� ����� ��, ȣ���Ѵ�. ���⿡�� ���� ���� ���۷�
	// ����� ������ �丸�� �����ϰ�, ���� ���� ���۴� ������ �ڿ��� �����Ѵ�.
	void SetDynamicVertexBuffer(ID3D12Resource* vb, const UINT32 vertexCount, const UINT32 vertexStride);

	// �ش� ���� ���� �� �ε��� ���۷� �޽��� �׸���.
	void Render(ID3D12GraphicsCommandList* commandList, const UINT32 instanceCount = 1, const bool isIndexed = true) const;

	// GPU���� ���ε尡 ���� �Ŀ� �� �޸𸮸� �����Ѵ�.
	void DisposeUploaders();

	void SetCollisionBoundingAsAABB(const XMFLOAT3& extents);
	void SetCollisionBoundingAsOBB(const XMFLOAT3& extents);
	void SetCollisionBoundingAsSphere(const float radius);

	void SetPrimitiveType(const D3D12_PRIMITIVE_TOPOLOGY primitiveType);
	CollisionType GetCollisionType() const;
	std::any GetCollisionBounding() const;

protected:
	// �޽��� Ÿ�Կ� ���� � �ٿ���� ������ �𸣴� anyŸ������ �����صд�.
	// ����� ������ any_cast�� �ش�Ǵ� Ÿ�Կ� ���� ĳ��Ʈ�Ͽ� ����Ѵ�.
	// mesh������ �浹 �ٿ��� �� ��ǥ�� �����̴�.
	std::any collisionBounding = nullptr;
	CollisionType collisionType = CollisionType::None;

private:
	// �ý��� �޸� ���纻. ����/���� ������ �������� �� �����Ƿ� ID3DBlob�� ����Ѵ�.
	Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCPU = nullptr;

	// ���� ����� ���� ���� �� �ε��� ���� ���ҽ�
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

	// gpu�� ������ ���ε� ����
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

	// ���� �信 ���� �ڷ�
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	UINT32 indexCount = 0;
	UINT32 vertexCount = 0;
};
