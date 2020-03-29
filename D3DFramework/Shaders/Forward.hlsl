#include "Common.hlsl"

struct VertexIn
{
	float3 posL     : POSITION;
    float3 normalL  : NORMAL;
	float3 tangentU : TANGENT;
	float3 binormalU: BINORMAL;
	float2 texC     : TEXCOORD;
};

struct VertexOut
{
	float4 posH      : SV_POSITION;
    float3 posW      : POSITION0;
    float3 normalW   : NORMAL;
	float3 tangentW  : TANGENT;
	float3 binormalW : BINORMAL;
	float2 texC      : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // World Space로 변환한다.
    float4 posW = mul(float4(vin.posL, 1.0f), gObjWorld);
    vout.posW = posW.xyz;

	// 동차 절단 공간으로 변환한다.
	vout.posH = mul(posW, gViewProj);

	// World Matrix에 비균등 비례가 없다고 가정하고 Normal을 변환한다.
	// 비균등 비례가 있다면 역전치 행렬을 사용해야 한다.
    vout.normalW = mul(vin.normalL, (float3x3)gObjWorld);
	vout.tangentW = mul(vin.tangentU, (float3x3)gObjWorld);
	vout.binormalW = mul(vin.binormalU, (float3x3)gObjWorld);
	
	// 출력 정점 특성들은 이후 삼각형을 따라 보간된다.
	vout.texC = mul(float4(vin.texC, 0.0f, 1.0f), GetMaterialTransform(gObjMaterialIndex)).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// 이 픽셀에 사용할 머터리얼 데이터를 가져온다.
	MaterialData materialData = gMaterialData[gObjMaterialIndex];

	float4 diffuse = materialData.diffuseAlbedo;
	float3 specular = materialData.specular;
	float roughness = materialData.roughness;
	uint diffuseMapIndex = materialData.diffuseMapIndex;
	uint normalMapIndex = materialData.normalMapIndex;

	if (diffuseMapIndex != DISABLED)
		diffuse *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.texC);

    // 텍스처 알파가 0.1보다 작으면 픽셀을 폐기한다. 
    clip(diffuse.a - 0.1f);

	// 법선을 보간하면 단위 길이가 아니게 될 수 있으므로 다시 정규화한다.
    pin.normalW = normalize(pin.normalW);
	pin.tangentW = normalize(pin.tangentW);
	pin.binormalW = normalize(pin.binormalW);
	float3 bumpedNormalW = pin.normalW;
	
	if (normalMapIndex != DISABLED)
	{
		float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.texC);

		// 노멀맵에서 Tangent Space의 노멀을 World Space의 노멀로 변환한다.
		float3 normalT = 2.0f * normalMapSample.rgb - 1.0f;
		float3x3 tbn = float3x3(pin.tangentW, pin.binormalW, pin.normalW);
		bumpedNormalW = mul(normalT, tbn);
		bumpedNormalW = normalize(bumpedNormalW);
	}

    // 조명되는 픽셀에서 눈으로의 벡터
	float3 toEyeW = gEyePosW - pin.posW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Diffuse를 전반적으로 밝혀주는 Ambient항
	float4 ambient = gAmbientLight * diffuse;

	// roughness와 normal를 이용하여 shininess를 계산한다.
	const float shininess = 1.0f - roughness;
    Material mat = { diffuse, specular, shininess };

	// Lighting을 실시한다.
    float4 directLight = ComputeShadowLighting(gLights, mat, pin.posW, bumpedNormalW, toEyeW);
	float4 litColor = ambient + directLight;

#ifdef SSAO
	// SSAO를 사용하는 경우 Ssao맵에서 차폐도를 가져와서 
	// 빛의 세기를 줄인다.
	float4 ssaoPosH = mul(float4(pin.posW, 1.0f), gViewProjTex);
	ssaoPosH /= ssaoPosH.w;
	float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, ssaoPosH.xy, 0.0f).r;
	litColor *= ambientAccess;
#endif

	// 분산 재질에서 알파를 가져온다.
	litColor.a = diffuse.a;

	if (gFogEnabled)
	{
		// 안개를 사용한다면 안개의 설정에 따라 색을 섞는다.
		litColor = FogBlend(litColor, distToEye);
	}

    return litColor;
}


