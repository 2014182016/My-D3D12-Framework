
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "pch.h"
#include "Enums.h"

struct WaveHeaderType
{
	char chunkId[4];
	unsigned long chunkSize;
	char format[4];
	char subChunkId[4];
	unsigned long subChunkSize;
	unsigned short audioFormat;
	unsigned short numChannels;
	unsigned long sampleRate;
	unsigned long bytesPerSecond;
	unsigned short blockAlign;
	unsigned short bitsPerSample;
	char dataChunkId[4];
	unsigned long dataSize;
};

struct Texture
{
	std::string mName;
	std::wstring mFilename;
	std::uint32_t mTextureIndex;

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

struct WidgetConstants
{
	std::uint32_t mMaterialIndex = -1;
	float mPadding0;
	float mPadding1;
	float mPadding2;
};


struct LightData
{
	DirectX::XMFLOAT4X4 mShadowTransform = DirectX::Identity4x4f();
	DirectX::XMFLOAT3 mStrength;
	float mFalloffStart;
	DirectX::XMFLOAT3 mDirection; 
	float mFalloffEnd; 
	DirectX::XMFLOAT3 mPosition; 
	float mSpotAngle;
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
	float mPadding1;
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
	float mFogStart = 5.0f; // Linear Fog
	float mFogRange = 150.0f; // Linear Fog
	float mFogDensity = 0.0f; // Exponential Fog
	std::uint32_t mFogEnabled = false;
	std::uint32_t mFogType = (std::uint32_t)FogType::Exponential;
	std::uint32_t mCurrentSkyCubeMapIndex;
	float mPadding2;
	float mPadding3;
};

struct MaterialData
{
	// 텍스쳐 매핑에 사용
	DirectX::XMFLOAT4X4 mMatTransform = DirectX::Identity4x4f();
	// 물체 표면의 전반적인 색을 나타내는 Diffuse
	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	// 물체 표면의 하이라이트를 나타내는 Specular
	DirectX::XMFLOAT3 mSpecular = { 0.01f, 0.01f, 0.01f };
	// 물체 표면의 거칠기를 나타내는 Roughness
	// 0 ~ 1 사이의 값을 가지며 이 값을 통해 표면의 거칠기를
	// 나타내는 m을 유도한다.
	float mRoughness = 0.25f;
	std::uint32_t mDiffuseMapIndex = -1;
	std::uint32_t mNormalMapIndex = -1;
	std::uint32_t mMaterialPad1;
	std::uint32_t mMaterialPad2;
};

struct ParticleConstants
{
	std::uint32_t mMaterialIndex;
	std::uint32_t mFacingCamera;
	float mPadding0;
	float mPadding1;
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

struct WidgetVertex
{
	WidgetVertex() = default;
	WidgetVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 tex) : mPos(pos), mTex(tex) { }
	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT2 mTex;
};

struct ParticleVertex
{
	ParticleVertex() = default;
	ParticleVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT4 color, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT2 size)
		: mPos(pos), mColor(color), mNormal(normal), mSize(size) { }
	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT4 mColor;
	DirectX::XMFLOAT3 mNormal;
	DirectX::XMFLOAT2 mSize;
};

struct LineVertex
{
	LineVertex() = default;
	LineVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT4 color) : mPosition(pos), mColor(color) { }
	DirectX::XMFLOAT3 mPosition;
	DirectX::XMFLOAT4 mColor;
};

#endif // STRUCTURES_H