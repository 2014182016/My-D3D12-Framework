#pragma once

#include "Component.h"
#include "Enumeration.h"

class Mesh : public Component
{
public:
	Mesh(std::string&& name);
	virtual ~Mesh();

public:
	void BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, void* vertices, UINT vertexCount, UINT vertexStride);
	void BuildIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::uint16_t* indices, UINT indexCount, UINT indexStride);
	void BuildCollisionBound(DirectX::XMFLOAT3* posPtr, size_t vertexCount, size_t stride,  CollisionType type);

	void SetDynamicVertexBuffer(ID3D12Resource* vb, UINT vertexCount, UINT vertexStride);

	void Render(ID3D12GraphicsCommandList* commandList, UINT instanceCount = 1, bool isIndexed = true) const;

	// GPU���� ���ε尡 ���� �Ŀ� �� �޸𸮸� �����Ѵ�.
	void DisposeUploaders();

	void SetCollisionBoundingAsAABB(DirectX::XMFLOAT3 extents);
	void SetCollisionBoundingAsOBB(DirectX::XMFLOAT3 extents);
	void SetCollisionBoundingAsSphere(float radius);

public:
	inline D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return mVertexBufferView; }
	inline D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return mIndexBufferView; }

	inline void SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY primitiveType) { mPrimitiveType = primitiveType; }

	inline CollisionType GetCollisionType() const { return mCollisionType; }
	inline std::any GetCollisionBounding() const { return mCollisionBounding; }

protected:
	// �޽��� Ÿ�Կ� ���� � �ٿ���� ������ �𸣴� anyŸ������ �����صд�.
	// ����� ������ any_cast�� �ش�Ǵ� Ÿ�Կ� ���� ĳ��Ʈ�Ͽ� ����Ѵ�.
	// mesh������ �浹 �ٿ��� �� ��ǥ�� �����̴�.
	std::any mCollisionBounding = nullptr;
	CollisionType mCollisionType = CollisionType::None;

private:
	// �ý��� �޸� ���纻. ����/���� ������ �������� �� �����Ƿ�
	// ID3DBlob�� ����Ѵ�.
	Microsoft::WRL::ComPtr<ID3DBlob> mVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mIndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferUploader = nullptr;

	// ���� �信 ���� �ڷ�
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

	UINT mIndexCount = 0;
	UINT mVertexCount = 0;

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};
