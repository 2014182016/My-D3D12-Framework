
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "pch.h"

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
	std::uint32_t mMaterialIndex;
	std::uint32_t mObjPad0;
	std::uint32_t mObjPad1;
	std::uint32_t mObjPad2;
};

struct InstanceConstants
{
	// �ν��Ͻ��ϱ� ���� ���� �ν��Ͻ� �ε���
	// �� ���ۿ� �ν��Ͻ��ϱ� ���� ������Ʈ �����͸� ��Ƴ���
	// mInstanceIndex�������� �� ������Ʈ�� �ν��Ͻ����� �׸��� ���� ���ȴ�.
	std::uint32_t mInstanceIndex = 0;
	std::uint32_t mOffsetIndex = 0;
	std::uint32_t mInstPad1;
	std::uint32_t mInstPad2;
};

struct DebugData
{
	DebugData() = default;
	DebugData(DirectX::XMFLOAT4X4 boundingWorld) : mBoundingWorld(boundingWorld) { }

	DirectX::XMFLOAT4X4 mBoundingWorld = DirectX::Identity4x4f();
};

struct LightData
{
	DirectX::XMFLOAT4X4 mWorld = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mViweProj = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mShadowTransform = DirectX::Identity4x4f();
	DirectX::XMFLOAT3 mStrength;
	float mFalloffStart; // point/spot light only
	DirectX::XMFLOAT3 mDirection; // directional/spot light only
	float mFalloffEnd; // point/spot light only
	DirectX::XMFLOAT3 mPosition; // point/spot light only
	float mSpotPower; // spot light only
	std::uint32_t mEnabled = false;
	std::uint32_t mSelected = false;
	std::uint32_t mType;
	float mPadding0;
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
	std::uint32_t mCurrentSkyCubeMapIndex;
	DirectX::XMFLOAT2 mRenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 mInvRenderTargetSize = { 0.0f, 0.0f };
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mTotalTime = 0.0f;
	float mDeltaTime = 0.0f;

	// �� ������ ����Ǵ� �������� Light
	DirectX::XMFLOAT4 mAmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// �Ȱ� �Ӽ�
	DirectX::XMFLOAT4 mFogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float mFogStart = 5.0f;
	float mFogRange = 150.0f;
	std::uint32_t mFogEnabled = false;
	float mPadding0;
};

struct MaterialData
{
	// �ؽ��� ���ο� ���
	DirectX::XMFLOAT4X4 mMatTransform = DirectX::Identity4x4f();
	// ��ü ǥ���� �������� ���� ��Ÿ���� Diffuse
	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	// ��ü ǥ���� ���̶���Ʈ�� ��Ÿ���� Specular
	DirectX::XMFLOAT3 mSpecular = { 0.01f, 0.01f, 0.01f };
	// ��ü ǥ���� ��ĥ�⸦ ��Ÿ���� Roughness
	// 0 ~ 1 ������ ���� ������ �� ���� ���� ǥ���� ��ĥ�⸦
	// ��Ÿ���� m�� �����Ѵ�.
	float mRoughness = 0.25f;
	std::uint32_t mDiffuseMapIndex = 0;
	std::uint32_t mNormalMapIndex = 0;
	std::uint32_t mMaterialPad1;
	std::uint32_t mMaterialPad2;
};

struct FaceIndex
{
	FaceIndex() = default;
	FaceIndex(std::uint16_t index1, std::uint16_t index2, std::uint16_t index3) :
		mPosIndex(index1), mNormalIndex(index2), mTexIndex(index3) { }

	bool operator==(const FaceIndex& rhs)
	{
		if (mPosIndex == rhs.mPosIndex && mNormalIndex == rhs.mNormalIndex && mTexIndex == rhs.mTexIndex)
			return true;
		return false;
	}

	std::uint16_t mPosIndex;
	std::uint16_t mNormalIndex;
	std::uint16_t mTexIndex;
};

struct VertexBasic
{
	VertexBasic() = default;
	VertexBasic(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT2 tex)
		: mPos(pos), mNormal(normal), mTexC(tex) { }

	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT3 mNormal;
	DirectX::XMFLOAT2 mTexC;
};

struct Vertex
{
	Vertex() = default;
	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT3 tangent, DirectX::XMFLOAT2 tex, 
		DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::White)
		: mPos(pos), mNormal(normal), mTangentU(tangent), mTexC(tex), mColor(color) { }

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT3 tangent, DirectX::XMFLOAT3 binormal, 
		DirectX::XMFLOAT2 tex, DirectX::XMFLOAT4 color = (DirectX::XMFLOAT4)DirectX::Colors::White)
		: mPos(pos), mNormal(normal), mTangentU(tangent), mBinormalU(binormal), mTexC(tex), mColor(color) { }

	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT3 mNormal;
	DirectX::XMFLOAT3 mTangentU;
	DirectX::XMFLOAT3 mBinormalU;
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

struct UIVertex
{
	UIVertex() = default;
	UIVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 tex) : mPos(pos), mTexC(tex) { }
	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT2 mTexC;
};

struct DebugVertex
{
	DebugVertex() = default;
	DebugVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT4 color) : mPosition(pos), mColor(color) { }
	DirectX::XMFLOAT3 mPosition;
	DirectX::XMFLOAT4 mColor;
};

#endif // STRUCTURES_H