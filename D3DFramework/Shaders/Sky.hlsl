#include "Common.hlsl"

static float4 gTopColor = float4(0.0f, 0.05f, 0.6f, 1.0f);
static float4 gCenterColor = float4(0.7f, 0.7f, 0.92f, 1.0f);
static float gSkyScale = 5000.0f;

struct VertexIn
{
	float3 posL : POSITION;
	float2 texC : TEXCOORD;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float3 posW : POSITION;
	float2 texC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// World Space로 변환한다.
	float4 posW = mul(float4(vin.posL, 1.0f), gObjWorld);
	vout.posW = posW.xyz;

	// z = w이면 해당 픽셀은 항상 far plane에 위치해있다.
	vout.posH = mul(posW, gViewProj).xyww;

	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
	vout.texC = mul(float4(vin.texC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;

	return vout;
}

[earlydepthstencil]
float4 PS(VertexOut pin) : SV_Target
{
	uint diffuseMapIndex = gMaterialData[gObjMaterialIndex].diffuseMapIndex;
	float diffuseIntensity = 1.0f;

	// 텍스처 배열의 텍스처를 동적으로 조회한다.
	if (diffuseMapIndex != DISABLED)
	{
		// 텍스처의 R값만을 가져와 구름 부분만을 렌더링하도록 한다.
		diffuseIntensity = gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC).r;
	}

	// 하늘의 높이는 [0, 1]사이로 정규화한다.
	float height = pin.posW.y / gSkyScale;
	if (height < 0.0f)
	{
		// 위와 아래는 대칭형태로 렌더링된다.
		height *= -1.0f;
	}
	
	// 높이에 따라 꼭대기 부분과 중심 부분의 색을 섞는다.
	float4 skyColor = lerp(gTopColor, gCenterColor, height);
	// 하늘에 구름을 렌더링한다.
	float4 outColor = skyColor + diffuseIntensity;

	return outColor;
}

