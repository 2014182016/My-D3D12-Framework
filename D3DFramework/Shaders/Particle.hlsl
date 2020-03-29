#include "Pass.hlsl"

static const float3 direction[8] =
{
	normalize(float3(1.0f, 1.0f, 1.0f)),
	normalize(float3(-1.0f, 1.0f, 1.0f)),
	normalize(float3(-1.0f, -1.0f, 1.0f)),
	normalize(float3(1.0f, -1.0f, 1.0f)),
	normalize(float3(1.0f, 1.0f, -1.0f)),
	normalize(float3(-1.0f, 1.0f, -1.0f)),
	normalize(float3(1.0f, -1.0f, -1.0f)),
	normalize(float3(-1.0f, -1.0f, -1.0f))
};

struct ParticleData
{
	float4 color;
	float3 position;
	float lifeTime;
	float3 direction;
	float speed;
	float2 size;
	bool isActive;
	float padding1;
};

cbuffer cbParticleConstants : register(b0)
{
	ParticleData gStart;
	ParticleData gEnd;
	float3 gEmitterLocation;
	uint gEnabledGravity;
	uint gMaxParticleNum;
	uint gParticleCount;
	uint gEmitNum;
	uint gTextureIndex;
}