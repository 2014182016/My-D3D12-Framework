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
	float4 mColor;
	float3 mPosition;
	float mLifeTime;
	float3 mDirection;
	float mSpeed;
	float2 mSize;
	bool mIsActive;
	float mPadding1;
};

cbuffer cbParticleConstants : register(b0)
{
	ParticleData gStart;
	ParticleData gEnd;
	float3 gEmitterLocation;
	float gpDeltaTime;
	uint gMaxParticleNum;
	uint gParticleCount;
	uint gEmitNum;
	uint gTextureIndex;
}
