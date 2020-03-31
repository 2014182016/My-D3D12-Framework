#pragma once

#include "Component.h"
#include <any>

/*
물체를 화면에 렌더링하기 위한 메쉬
정점 버퍼 및 인덱스 버퍼를 생성하고, 그린다.
*/
class Mesh : public Component
{
public:
	Mesh(std::string&& name);
	virtual ~Mesh();

public:
	// 정점 버퍼를 생성한다.
	void BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, void* vertices, 
		const UINT32 vertexCount, const UINT32 vertexStride);
	// 인덱스 버퍼를 생성한다.
	void BuildIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT16* indices,
		const UINT32 indexCount, const UINT32 indexStride);
	// 충돌 타입에 따라 바운드를 생성한다.
	void BuildCollisionBound(XMFLOAT3* posPtr, const UINT32 vertexCount, const UINT32 stride, 
		const CollisionType type);
	// 동적 정점 버퍼를 사용할 때, 호출한다. 여기에는 동점 정점 버퍼로
	// 사용할 서술자 뷰만을 정의하고, 실제 정점 버퍼는 프레임 자원에 선언한다.
	void SetDynamicVertexBuffer(ID3D12Resource* vb, const UINT32 vertexCount, const UINT32 vertexStride);

	// 해당 정점 버퍼 및 인덱스 버퍼로 메쉬를 그린다.
	void Render(ID3D12GraphicsCommandList* commandList, const UINT32 instanceCount = 1, const bool isIndexed = true) const;

	// GPU로의 업로드가 끝난 후에 이 메모리를 해제한다.
	void DisposeUploaders();

	void SetCollisionBoundingAsAABB(const XMFLOAT3& extents);
	void SetCollisionBoundingAsOBB(const XMFLOAT3& extents);
	void SetCollisionBoundingAsSphere(const float radius);

	void SetPrimitiveType(const D3D12_PRIMITIVE_TOPOLOGY primitiveType);
	CollisionType GetCollisionType() const;
	std::any GetCollisionBounding() const;

protected:
	// 메쉬의 타입에 따라 어떤 바운딩이 생길지 모르니 any타입으로 선언해둔다.
	// 사용할 때에는 any_cast로 해당되는 타입에 따라 캐스트하여 사용한다.
	// mesh에서의 충돌 바운드는 모델 좌표계 기준이다.
	std::any collisionBounding = nullptr;
	CollisionType collisionType = CollisionType::None;

private:
	// 시스템 메모리 복사본. 정점/색인 형식이 벙용적일 수 있으므로 ID3DBlob를 사용한다.
	Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCPU = nullptr;

	// 실제 사용할 정점 버퍼 및 인덱스 버퍼 리소스
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

	// gpu에 복사할 업로드 버퍼
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

	// 버퍼 뷰에 관한 자료
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	UINT32 indexCount = 0;
	UINT32 vertexCount = 0;
};
