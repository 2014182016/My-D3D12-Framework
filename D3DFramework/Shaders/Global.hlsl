#define DISABLED -1

#define TEX_NUM 12

#define FOG_LINEAR 0
#define FOG_EXPONENTIAL 1
#define FOG_EXPONENTIAL2 2 

#define LIGHT_NUM 1

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

struct MaterialData
{
	float4x4 mMatTransform;
	float4   mDiffuseAlbedo;
	float3   mSpecular;
	float    mRoughness;
	uint     mDiffuseMapIndex;
	uint     mNormalMapIndex;
	uint     mHeightMapIndex;
	uint     mMatPad0;
};
