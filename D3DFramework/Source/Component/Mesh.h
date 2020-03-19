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

	// GPU로의 업로드가 끝난 후에 이 메모리를 해제한다.
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
	// 메쉬의 타입에 따라 어떤 바운딩이 생길지 모르니 any타입으로 선언해둔다.
	// 사용할 때에는 any_cast로 해당되는 타입에 따라 캐스트하여 사용한다.
	// mesh에서의 충돌 바운드는 모델 좌표계 기준이다.
	std::any mCollisionBounding = nullptr;
	CollisionType mCollisionType = CollisionType::None;

private:
	// 시스템 메모리 복사본. 정점/색인 형식이 벙용적일 수 있으므로
	// ID3DBlob를 사용한다.
	Microsoft::WRL::ComPtr<ID3DBlob> mVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mIndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferUploader = nullptr;

	// 버퍼 뷰에 관한 자료
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

	UINT mIndexCount = 0;
	UINT mVertexCount = 0;

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};
