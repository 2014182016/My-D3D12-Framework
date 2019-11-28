
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "pch.h"

#define MAX_LIGHT 1

struct Texture
{
	std::string mName;
	std::wstring mFilename;

	Microsoft::WRL::ComPtr<ID3D12Resource> mResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadHeap = nullptr;
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 mWorld = DirectX::Identity4x4f();
	UINT mMaterialIndex;
	UINT mObjPad0;
	UINT mObjPad1;
	UINT mObjPad2;
};

struct InstanceConstants
{
	// 인스턴싱하기 위한 시작 인스턴스 인덱스
	// 한 버퍼에 인스턴싱하기 위한 오브젝트 데이터를 모아놓고
	// mInstanceIndex에서부터 각 오브젝트를 인스턴싱으로 그리기 위해 사용된다.
	UINT mInstanceIndex = 0;
	UINT mDebugInstanceIndex = 0;
	UINT mInstPad1;
	UINT mInstPad2;
};

struct DebugData
{
	DebugData() = default;
	DebugData(DirectX::XMFLOAT4X4 boundingWorld) : mBoundingWorld(boundingWorld) { }

	DirectX::XMFLOAT4X4 mBoundingWorld = DirectX::Identity4x4f();
};

struct LightConstants
{
	DirectX::XMFLOAT3 mStrength;
	float mFalloffStart; // point/spot light only
	DirectX::XMFLOAT3 mDirection; // directional/spot light only
	float mFalloffEnd; // point/spot light only
	DirectX::XMFLOAT3 mPosition; // point/spot light only
	float mSpotPower; // spot light only
	std::uint32_t mEnabled = false;
	std::uint32_t mSelected = false;
	std::uint32_t mType;
	float mPadding;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 mView = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mInvView = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mProj = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mInvProj = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mViewProj = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mInvViewProj = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mIdentity = DirectX::Identity4x4f();
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
	std::uint32_t mFogEnabled = false;
	float mPadding0;

	LightConstants mLights[MAX_LIGHT];
};

struct MaterialData
{
	// 물체 표면의 전반적인 색을 나타내는 Diffuse
	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	// 물체 표면의 하이라이트를 나타내는 Specular
	DirectX::XMFLOAT3 mSpecular = { 0.01f, 0.01f, 0.01f };
	// 물체 표면의 거칠기를 나타내는 Roughness
	// 0 ~ 1 사이의 값을 가지며 이 값을 통해 표면의 거칠기를
	// 나타내는 m을 유도한다.
	float mRoughness = 0.25f;

	// 텍스쳐 매핑에 사용
	DirectX::XMFLOAT4X4 mMatTransform = DirectX::Identity4x4f();

	UINT mDiffuseMapIndex = 0;
	UINT mNormalMapIndex = 0;
	UINT mMaterialPad1;
	UINT mMaterialPad2;
};

struct Vertex
{
	Vertex() = default;
	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT3 tangent, DirectX::XMFLOAT2 tex, 
		const DirectX::XMVECTORF32 color = DirectX::Colors::White)
		: mPos(pos), mNormal(normal), mTangentU(tangent), mTexC(tex), mColor(color) { }

	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT3 mNormal;
	DirectX::XMFLOAT3 mTangentU;
	DirectX::XMFLOAT2 mTexC;
	DirectX::XMFLOAT4 mColor;
};

struct BillboardVertex
{
	BillboardVertex() = default;
	BillboardVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size) : mPos(pos), mSize(size) { }

	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT2 mSize;
};

struct DebugVertex
{
	DebugVertex() = default;
	DebugVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT4 color) : mPosition(pos), mColor(color) { }
	DirectX::XMFLOAT3 mPosition;
	DirectX::XMFLOAT4 mColor;
};

#endif // STRUCTURES_H