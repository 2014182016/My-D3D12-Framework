
#ifndef GLOBAL_H
#define GLOBAL_H

#include "pch.h"
#include "D3DUtil.h"

#define SWAP_CHAIN_BUFFER_COUNT 2
#define NUM_FRAME_RESOURCES 3

#define MAX_LIGHTS 16
#define DIRECTIONAL_LIGHTS 3
#define POINT_LIGHTS 0
#define SPOT_LIGHTS 0

// 이 구조체는 MeshGeometry가 대표하는 기하구조 그룹(메시)의 부분 구간, 부분 
// 메시를 정의한다. 부분 메시는 하나의 정점/색인 버퍼에 여러 개의 기하구조가
// 들어 있는 경우에 쓰인다. 이 구조체는 정점/색인 버퍼에 저장된 메시의 부분
// 메시를 그리는 데 필요한 오프셋들과 자료를 제공한다.
struct SubmeshGeometry
{
	UINT mIndexCount = 0;
	UINT mStartIndexLocation = 0;
	INT mBaseVertexLocation = 0;

	// 물체의 충돌을 위한 BoundingBox
	DirectX::BoundingBox mBounds;
};

struct MeshGeometry
{
	std::string mName;

	// 시스템 메모리 복사본. 정점/색인 형식이 벙용적일 수 있으므로
	// ID3DBlob를 사용한다.
	Microsoft::WRL::ComPtr<ID3DBlob> mVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mIndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferUploader = nullptr;

	// 버퍼들에 관한 자료
	UINT mVertexByteStride = 0;
	UINT mVertexBufferByteSize = 0;
	DXGI_FORMAT mIndexFormat = DXGI_FORMAT_R16_UINT;
	UINT mIndexBufferByteSize = 0;

	// 한 MeshGeomoetry 인스턴스의 한 정점/색인 버퍼에 여러 개의 기하구조를
	// 담을 수 있다. 부분 메시들을 개별적으로 그릴 수 있도록, 부분 메시 기하구조들을
	// 컨테이너에 담아 둔다.
	std::unordered_map<std::string, SubmeshGeometry> mDrawArgs;

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = mVertexByteStride;
		vbv.SizeInBytes = mVertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = mIndexFormat;
		ibv.SizeInBytes = mIndexBufferByteSize;

		return ibv;
	}

	// GPU로의 업로드가 끝난 후에 이 메모리를 해제한다.
	void DisposeUploaders()
	{
		mVertexBufferUploader = nullptr;
		mIndexBufferUploader = nullptr;
	}
};

struct LightConstants
{
	DirectX::XMFLOAT3 mStrength = { 0.5f, 0.5f, 0.5f };
	float mFalloffStart = 1.0f;                          // point/spot light only
	DirectX::XMFLOAT3 mDirection = { 0.0f, -1.0f, 0.0f };// directional/spot light only
	float mFalloffEnd = 10.0f;                           // point/spot light only
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };  // point/spot light only
	float mSpotPower = 64.0f;                            // spot light only
};

struct Material
{
	std::string mName;

	// 이 머터리얼에 일치하는 상수 버퍼에서의 인덱스
	int mMatCBIndex = -1;

	// 디퓨즈 텍스처를 위한 SRV 힙에서의 인덱스
	int mDiffuseSrvHeapIndex = -1;

	// 노말 텍스처를 위한 SRV 힙에서의 인덱스
	int mNormalSrvHeapIndex = -1;

	// 재질이 변해서 해당 상수 버퍼를 갱신해야 하는지의 여부를 나타내는 더러움 플래그
	// FrameResource마다 물체의 상수 버퍼가 있으므로, FrameResouce마다 갱신을 적용해야 한다.
	// 따라서, 물체의 자료를 수정할 때에는 반드시 mNumFramesDiry = NUM_FRAME_RESOURCES을
	// 적용해야 한다.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;

	// 쉐이딩에서 사용되는 MaterialConstants 자료들
	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 mFresnelR0 = { 0.01f, 0.01f, 0.01f };
	float mRoughness = .25f;
	DirectX::XMFLOAT4X4 mMatTransform = D3DUtil::Identity4x4f();
};

struct Texture
{
	std::string mName;
	std::wstring mFilename;

	Microsoft::WRL::ComPtr<ID3D12Resource> mResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadHeap = nullptr;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Billborad,
	Wireframe,
	Debug,
	Count
};

// 하나의 물체를 그리는 데 필요한 매개변수들을 담는 가벼운 구조체
struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;

	// 세계 공간을 기준으로 물체의 지역 공간을 서술하는 세계 행렬
	// 이 행렬은 세계 공간 안에서의 물체의 위치, 방향, 크기를 결정한다.
	DirectX::XMFLOAT4X4 mWorld = D3DUtil::Identity4x4f();

	// 텍스처의 이동, 회전, 크기를 지정한다.
	DirectX::XMFLOAT4X4 mTexTransform = D3DUtil::Identity4x4f();

	// 재질이 변해서 해당 상수 버퍼를 갱신해야 하는지의 여부를 나타내는 더러움 플래그
	// FrameResource마다 물체의 상수 버퍼가 있으므로, FrameResouce마다 갱신을 적용해야 한다.
	// 따라서, 물체의 자료를 수정할 때에는 반드시 mNumFramesDiry = NUM_FRAME_RESOURCES을
	// 적용해야 한다.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;

	// 이 렌더 항목의 물체 상수 버퍼에 해당하는 GPU 상수 버퍼의 색인
	UINT mObjCBIndex = -1;

	Material* mMat = nullptr;
	MeshGeometry* mGeo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	SubmeshGeometry mSubmesh;
};

/******************************
/////// 상수 버퍼 자료 ////////
******************************/

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 mWorld = D3DUtil::Identity4x4f();
	DirectX::XMFLOAT4X4 mTexTransform = D3DUtil::Identity4x4f();
	UINT mMaterialIndex;
	UINT mObjPad0;
	UINT mObjPad1;
	UINT mObjPad2;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 mView = D3DUtil::Identity4x4f();
	DirectX::XMFLOAT4X4 mInvView = D3DUtil::Identity4x4f();
	DirectX::XMFLOAT4X4 mProj = D3DUtil::Identity4x4f();
	DirectX::XMFLOAT4X4 mInvProj = D3DUtil::Identity4x4f();
	DirectX::XMFLOAT4X4 mViewProj = D3DUtil::Identity4x4f();
	DirectX::XMFLOAT4X4 mInvViewProj = D3DUtil::Identity4x4f();
	DirectX::XMFLOAT3 mEyePosW = { 0.0f, 0.0f, 0.0f };
	float mcbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 mRenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 mInvRenderTargetSize = { 0.0f, 0.0f };
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mTotalTime = 0.0f;
	float mDeltaTime = 0.0f;

	// 맵 전역에 적용되는 전반적인 Light
	DirectX::XMFLOAT4 mAmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// 안개 속성
	DirectX::XMFLOAT4 mFogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float mFogStart = 5.0f;
	float mFogRange = 150.0f;
	DirectX::XMFLOAT2 mcbPerObjectPad2;

	LightConstants mLights[MAX_LIGHTS];
};

struct MaterialData
{
	// 물체 표면의 전반적인 색을 나타내는 Diffuse
	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	// 물체 표면의 하이라이트를 나타내는 Specular
	DirectX::XMFLOAT3 mFresnelR0 = { 0.01f, 0.01f, 0.01f };
	// 물체 표면의 거칠기를 나타내는 Roughness
	// 0 ~ 1 사이의 값을 가지며 이 값을 통해 표면의 거칠기를
	// 나타내는 m을 유도한다.
	float mRoughness = 0.25f;

	// 텍스쳐 매핑에 사용
	DirectX::XMFLOAT4X4 mMatTransform = D3DUtil::Identity4x4f();

	UINT mDiffuseMapIndex = 0;
	UINT mNormalMapIndex = 0;
	UINT mMaterialPad1;
	UINT mMaterialPad2;
};

struct Vertex
{
	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT3 mNormal;
	DirectX::XMFLOAT2 mTexC;
	DirectX::XMFLOAT3 mTangentU;
};

#endif // GLOBAL_H