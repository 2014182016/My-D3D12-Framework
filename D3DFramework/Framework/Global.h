
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

// �� ����ü�� MeshGeometry�� ��ǥ�ϴ� ���ϱ��� �׷�(�޽�)�� �κ� ����, �κ� 
// �޽ø� �����Ѵ�. �κ� �޽ô� �ϳ��� ����/���� ���ۿ� ���� ���� ���ϱ�����
// ��� �ִ� ��쿡 ���δ�. �� ����ü�� ����/���� ���ۿ� ����� �޽��� �κ�
// �޽ø� �׸��� �� �ʿ��� �����µ�� �ڷḦ �����Ѵ�.
struct SubmeshGeometry
{
	UINT mIndexCount = 0;
	UINT mStartIndexLocation = 0;
	INT mBaseVertexLocation = 0;

	// ��ü�� �浹�� ���� BoundingBox
	DirectX::BoundingBox mBounds;
};

struct MeshGeometry
{
	std::string mName;

	// �ý��� �޸� ���纻. ����/���� ������ �������� �� �����Ƿ�
	// ID3DBlob�� ����Ѵ�.
	Microsoft::WRL::ComPtr<ID3DBlob> mVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mIndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferUploader = nullptr;

	// ���۵鿡 ���� �ڷ�
	UINT mVertexByteStride = 0;
	UINT mVertexBufferByteSize = 0;
	DXGI_FORMAT mIndexFormat = DXGI_FORMAT_R16_UINT;
	UINT mIndexBufferByteSize = 0;

	// �� MeshGeomoetry �ν��Ͻ��� �� ����/���� ���ۿ� ���� ���� ���ϱ�����
	// ���� �� �ִ�. �κ� �޽õ��� ���������� �׸� �� �ֵ���, �κ� �޽� ���ϱ�������
	// �����̳ʿ� ��� �д�.
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

	// GPU���� ���ε尡 ���� �Ŀ� �� �޸𸮸� �����Ѵ�.
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

	// �� ���͸��� ��ġ�ϴ� ��� ���ۿ����� �ε���
	int mMatCBIndex = -1;

	// ��ǻ�� �ؽ�ó�� ���� SRV �������� �ε���
	int mDiffuseSrvHeapIndex = -1;

	// �븻 �ؽ�ó�� ���� SRV �������� �ε���
	int mNormalSrvHeapIndex = -1;

	// ������ ���ؼ� �ش� ��� ���۸� �����ؾ� �ϴ����� ���θ� ��Ÿ���� ������ �÷���
	// FrameResource���� ��ü�� ��� ���۰� �����Ƿ�, FrameResouce���� ������ �����ؾ� �Ѵ�.
	// ����, ��ü�� �ڷḦ ������ ������ �ݵ�� mNumFramesDiry = NUM_FRAME_RESOURCES��
	// �����ؾ� �Ѵ�.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;

	// ���̵����� ���Ǵ� MaterialConstants �ڷ��
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

// �ϳ��� ��ü�� �׸��� �� �ʿ��� �Ű��������� ��� ������ ����ü
struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;

	// ���� ������ �������� ��ü�� ���� ������ �����ϴ� ���� ���
	// �� ����� ���� ���� �ȿ����� ��ü�� ��ġ, ����, ũ�⸦ �����Ѵ�.
	DirectX::XMFLOAT4X4 mWorld = D3DUtil::Identity4x4f();

	// �ؽ�ó�� �̵�, ȸ��, ũ�⸦ �����Ѵ�.
	DirectX::XMFLOAT4X4 mTexTransform = D3DUtil::Identity4x4f();

	// ������ ���ؼ� �ش� ��� ���۸� �����ؾ� �ϴ����� ���θ� ��Ÿ���� ������ �÷���
	// FrameResource���� ��ü�� ��� ���۰� �����Ƿ�, FrameResouce���� ������ �����ؾ� �Ѵ�.
	// ����, ��ü�� �ڷḦ ������ ������ �ݵ�� mNumFramesDiry = NUM_FRAME_RESOURCES��
	// �����ؾ� �Ѵ�.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;

	// �� ���� �׸��� ��ü ��� ���ۿ� �ش��ϴ� GPU ��� ������ ����
	UINT mObjCBIndex = -1;

	Material* mMat = nullptr;
	MeshGeometry* mGeo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	SubmeshGeometry mSubmesh;
};

/******************************
/////// ��� ���� �ڷ� ////////
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

	// �� ������ ����Ǵ� �������� Light
	DirectX::XMFLOAT4 mAmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// �Ȱ� �Ӽ�
	DirectX::XMFLOAT4 mFogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float mFogStart = 5.0f;
	float mFogRange = 150.0f;
	DirectX::XMFLOAT2 mcbPerObjectPad2;

	LightConstants mLights[MAX_LIGHTS];
};

struct MaterialData
{
	// ��ü ǥ���� �������� ���� ��Ÿ���� Diffuse
	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	// ��ü ǥ���� ���̶���Ʈ�� ��Ÿ���� Specular
	DirectX::XMFLOAT3 mFresnelR0 = { 0.01f, 0.01f, 0.01f };
	// ��ü ǥ���� ��ĥ�⸦ ��Ÿ���� Roughness
	// 0 ~ 1 ������ ���� ������ �� ���� ���� ǥ���� ��ĥ�⸦
	// ��Ÿ���� m�� �����Ѵ�.
	float mRoughness = 0.25f;

	// �ؽ��� ���ο� ���
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