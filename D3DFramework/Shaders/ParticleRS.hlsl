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
	uint gEnabledGravity;
	uint gMaxParticleNum;
	uint gParticleCount;
	uint gEmitNum;
	uint gTextureIndex;
}

cbuffer cbPass : register(b1)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float4x4 gProjTex;
	float4x4 gViewProjTex;
	float4x4 gIdentity;
	float4 gAmbientLight;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float3 gEyePosW;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	bool gFogEnabled;
	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float gFogDensity;
	uint gFogType;
};