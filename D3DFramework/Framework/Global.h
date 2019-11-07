
#ifndef GLOBAL_H
#define GLOBAL_H

#include "pch.h"
#include "D3DUtil.h"

#define MAX_LIGHTS 16

enum class RenderLayer : int
{
	NotRender = 0,
	Opaque,
	Transparent,
	AlphaTested,
	Billborad,
	Debug,
	Count
};

struct Texture
{
	std::string mName;
	std::wstring mFilename;

	Microsoft::WRL::ComPtr<ID3D12Resource> mResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadHeap = nullptr;
};

struct Material
{
public:
	Material(std::string name);
	~Material() { }

public:
	std::string mName;

	DirectX::XMFLOAT4X4 mMatTransform = D3DUtil::Identity4x4f();

	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 mSpecular = { 0.01f, 0.01f, 0.01f };
	float mRoughness = .25f;

	// ��ǻ�� �ؽ�ó�� ���� SRV �������� �ε���
	int mDiffuseSrvHeapIndex = -1;
	// �븻 �ؽ�ó�� ���� SRV �������� �ε���
	int mNormalSrvHeapIndex = -1;

	// ���͸����� ������ ������ ������ ��� ���ۿ����� �ε���
	static int mCurrentIndex;
	// �� ���͸��� ��ġ�ϴ� ��� ���ۿ����� �ε���
	int mMatCBIndex = -1;

	// ������ ���ؼ� �ش� ��� ���۸� �����ؾ� �ϴ����� ���θ� ��Ÿ���� ������ �÷���
	// FrameResource���� ��ü�� ��� ���۰� �����Ƿ�, FrameResouce���� ������ �����ؾ� �Ѵ�.
	// ����, ��ü�� �ڷḦ ������ ������ �ݵ�� mNumFramesDiry = NUM_FRAME_RESOURCES��
	// �����ؾ� �Ѵ�.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;
};

// �� ����ü�� MeshGeometry�� ��ǥ�ϴ� ���ϱ��� �׷�(�޽�)�� �κ� ����, �κ� 
// �޽ø� �����Ѵ�. �κ� �޽ô� �ϳ��� ����/���� ���ۿ� ���� ���� ���ϱ�����
// ��� �ִ� ��쿡 ���δ�. �� ����ü�� ����/���� ���ۿ� ����� �޽��� �κ�
// �޽ø� �׸��� �� �ʿ��� �����µ�� �ڷḦ �����Ѵ�.
struct SubmeshGeometry
{
public:
	UINT mIndexCount = 0;
	UINT mStartIndexLocation = 0;
	INT mBaseVertexLocation = 0;
};

struct MeshGeometry
{
public:
	MeshGeometry(std::string name) : mName(name) { }
	~MeshGeometry() { }

public:
	void BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
		std::vector<struct Vertex>& vertices, std::vector<std::uint16_t>& indices);

	void Render(ID3D12GraphicsCommandList* commandList) const;

	// GPU���� ���ε尡 ���� �Ŀ� �� �޸𸮸� �����Ѵ�.
	void DisposeUploaders();

public:
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

	// ���� �信 ���� �ڷ�
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

	UINT mIndexCount = 0;

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	/*
	// �� MeshGeomoetry �ν��Ͻ��� �� ����/���� ���ۿ� ���� ���� ���ϱ�����
	// ���� �� �ִ�. �κ� �޽õ��� ���������� �׸� �� �ֵ���, �κ� �޽� ���ϱ�������
	// �����̳ʿ� ��� �д�.
	std::unordered_map<std::string, SubmeshGeometry> mDrawArgs;
	*/

	DirectX::BoundingBox mBounds;

	// ������ ���ؼ� �ش� ��� ���۸� �����ؾ� �ϴ����� ���θ� ��Ÿ���� ������ �÷���
	// FrameResource���� ��ü�� ��� ���۰� �����Ƿ�, FrameResouce���� ������ �����ؾ� �Ѵ�.
	// ����, ��ü�� �ڷḦ ������ ������ �ݵ�� mNumFramesDiry = NUM_FRAME_RESOURCES��
	// �����ؾ� �Ѵ�.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;
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

struct LightConstants
{
	DirectX::XMFLOAT3 mStrength = { 0.5f, 0.5f, 0.5f };
	float mFalloffStart = 1.0f;                          // point/spot light only
	DirectX::XMFLOAT3 mDirection = { 0.0f, -1.0f, 0.0f };// directional/spot light only
	float mFalloffEnd = 10.0f;                           // point/spot light only
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };  // point/spot light only
	float mSpotPower = 64.0f;                            // spot light only
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
	DirectX::XMFLOAT3 mSpecular = { 0.01f, 0.01f, 0.01f };
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
	Vertex() { }
	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT2 tex, DirectX::XMFLOAT3 tangent) 
		: mPos(pos), mNormal(normal), mTexC(tex), mTangentU(tangent) { }

	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT3 mNormal;
	DirectX::XMFLOAT2 mTexC;
	DirectX::XMFLOAT3 mTangentU;
};

struct BillboardVertex
{
	BillboardVertex() { }
	BillboardVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 size) : mPos(pos), mSize(size) { }

	DirectX::XMFLOAT3 mPos;
	DirectX::XMFLOAT2 mSize;
};

#endif // GLOBAL_H