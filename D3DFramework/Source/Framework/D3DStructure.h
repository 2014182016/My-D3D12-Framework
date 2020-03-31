#pragma once

#include "Vector.h"
#include <basetsd.h>

struct ObjectConstants
{
	XMFLOAT4X4 world = Matrix4x4::Identity();
	UINT32 materialIndex;
	UINT32 objPad0;
	UINT32 objPad1;
	UINT32 objPad2;
};

struct LightData
{
	XMFLOAT4X4 shadowTransform = Matrix4x4::Identity();
	XMFLOAT3 strength;
	float falloffStart;
	XMFLOAT3 direction;
	float falloffEnd;
	XMFLOAT3 position;
	float spotAngle;
	UINT32 enabled = false;
	UINT32 selected = false;
	UINT32 type;
	float padding0;
};

struct PassConstants
{
	XMFLOAT4X4 view = Matrix4x4::Identity();
	XMFLOAT4X4 invView = Matrix4x4::Identity();
	XMFLOAT4X4 proj = Matrix4x4::Identity();
	XMFLOAT4X4 invProj = Matrix4x4::Identity();
	XMFLOAT4X4 viewProj = Matrix4x4::Identity();
	XMFLOAT4X4 invViewProj = Matrix4x4::Identity();
	XMFLOAT4X4 projTex = Matrix4x4::Identity();
	XMFLOAT4X4 viewProjTex = Matrix4x4::Identity();
	XMFLOAT4X4 identity = Matrix4x4::Identity();
	XMFLOAT4 ambientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMFLOAT2 renderTargetSize = { 0.0f, 0.0f };
	XMFLOAT2 invRenderTargetSize = { 0.0f, 0.0f };
	XMFLOAT3 eyePosW = { 0.0f, 0.0f, 0.0f };
	float nearZ = 0.0f;
	float farZ = 0.0f;
	float totalTime = 0.0f;
	float deltaTime = 0.0f;

	UINT32 fogEnabled = false;
	XMFLOAT4 fogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float fogStart = 5.0f; // Linear Fog
	float fogRange = 150.0f; // Linear Fog
	float fogDensity = 0.0f; // Exponential Fog
	UINT32 fogType = 1;
};

struct SsaoConstants
{
	XMFLOAT4 offsetVectors[14];
	XMFLOAT4 blurWeights[3];

	float occlusionRadius = 0.5f;
	float occlusionFadeStart = 0.2f;
	float occlusionFadeEnd = 2.0f;
	float surfaceEpsilon = 0.05f;

	XMFLOAT2 renderTargetInvSize;
	float ssaoContrast;
	float padding0;
};

struct SsrConstants
{
	float maxDistance;
	float thickness;
	INT32 rayTraceStep;
	INT32 binaryStep;
	XMFLOAT2 screenEdgeFadeStart;
	float strideCutoff;
	float resolutuon;
};

struct ParticleData
{
	XMFLOAT4 color;
	XMFLOAT3 position;
	float lifeTime = 0.0f;
	XMFLOAT3 direction;
	float speed;
	XMFLOAT2 size;
	bool isActive = false;
	float padding1;
};

struct ParticleConstants
{
	ParticleData start;
	ParticleData end;
	XMFLOAT3 emitterLocation;
	UINT32 enabledGravity;
	UINT32 maxParticleNum;
	UINT32 particleCount;
	UINT32 emitNum;
	UINT32 textureIndex;
};

struct TerrainConstants
{
	XMFLOAT4X4 terrainWorld;
	XMFLOAT2 pixelDimesion;
	XMFLOAT2 geometryDimension;
	float maxLOD;
	float minLOD;
	float minDistance;
	float maxDistance;
	float heightScale;
	float texScale;
	UINT32 materialIndex;
	float padding0;
};

struct MaterialData
{
	XMFLOAT4X4 matTransform = Matrix4x4::Identity();
	XMFLOAT4 diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 specular = { 0.01f, 0.01f, 0.01f };
	float roughness = 0.25f;

	UINT32 diffuseMapIndex = -1;
	UINT32 normalMapIndex = -1;
	UINT32 heightMapIndex = -1;
	UINT32 materialPad2;
};

struct FaceIndex
{
	FaceIndex() = default;
	FaceIndex(const UINT16 index1, const UINT16 index2, const UINT16 index3) :
		posIndex(index1), normalIndex(index2), texIndex(index3) { }

	bool operator==(const FaceIndex& rhs)
	{
		if (posIndex == rhs.posIndex && normalIndex == rhs.normalIndex && texIndex == rhs.texIndex)
			return true;
		return false;
	}

	UINT16 posIndex;
	UINT16 normalIndex;
	UINT16 texIndex;
};

struct VertexBasic
{
	VertexBasic() = default;
	VertexBasic(const XMFLOAT3& ipos, const XMFLOAT3& inormal, const XMFLOAT2& itex)
		: pos(ipos), normal(inormal), texC(itex) { }

	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 texC;
};

struct Vertex
{
	Vertex() = default;
	Vertex(const XMFLOAT3& ipos, const XMFLOAT3& inormal, const XMFLOAT3& itangent, const XMFLOAT2& itex,
		const XMFLOAT4& icolor = { 1.0f, 1.0f, 1.0f, 1.0f })
		: pos(ipos), normal(inormal), tangentU(itangent), texC(itex), color(icolor) { }

	Vertex(const XMFLOAT3& ipos, const XMFLOAT3& inormal, const XMFLOAT3& itangent, const XMFLOAT3& ibinormal,
		const XMFLOAT2& itex, const XMFLOAT4& icolor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f))
		: pos(ipos), normal(inormal), tangentU(itangent), binormalU(ibinormal), texC(itex), color(icolor) { }

	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT3 tangentU;
	XMFLOAT3 binormalU;
	XMFLOAT2 texC;
	XMFLOAT4 color;
};

struct BillboardVertex
{
	BillboardVertex() = default;
	BillboardVertex(const XMFLOAT3& ipos, const XMFLOAT2& isize) : pos(ipos), size(isize) { }

	XMFLOAT3 pos;
	XMFLOAT2 size;
};

struct WidgetVertex
{
	WidgetVertex() = default;
	WidgetVertex(const XMFLOAT3& ipos, const XMFLOAT2& itex) : pos(ipos), tex(itex) { }

	XMFLOAT3 pos;
	XMFLOAT2 tex;
};

struct LineVertex
{
	LineVertex() = default;
	LineVertex(const XMFLOAT3& ipos, const XMFLOAT4& icolor) : pos(ipos), color(icolor) { }

	XMFLOAT3 pos;
	XMFLOAT4 color;
};

struct TerrainVertex
{
	TerrainVertex() = default;
	TerrainVertex(const XMFLOAT3& ipos, const XMFLOAT2&itex) : pos(ipos), tex(itex) { }
	XMFLOAT3 pos;
	XMFLOAT2 tex;
};