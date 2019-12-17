#include "pch.h"
#include "SpotLight.h"
#include "Structures.h"

using namespace DirectX;

SpotLight::SpotLight(std::string&& name) : Light(std::move(name))
{
	mLightType = LightType::SpotLight;
}

SpotLight::~SpotLight() { }

void SpotLight::SetLightData(LightData& lightData, const BoundingSphere& sceneBounding)
{
	// NDC 공간 [-1, 1]^2을 텍스처 공간 [0, 1]^2으로 변환하는 행렬
	static XMMATRIX toTextureTransform(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	__super::SetLightData(lightData, sceneBounding);

	lightData.mDirection = GetLook();
	lightData.mFalloffStart = mFalloffStart;
	lightData.mFalloffEnd = mFalloffEnd;
	lightData.mSpotPower = mSpotPower;

	// 그림자 맵에 사용하기 위한 행렬을 계산한다.
	XMVECTOR lightDir = XMLoadFloat3(&GetLook());
	XMVECTOR lightPos = XMLoadFloat3(&GetPosition());
	XMVECTOR targetPos = XMVectorMultiplyAdd(lightDir, XMVectorReplicate(mFalloffEnd), lightPos);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);
	XMMATRIX lightProj = XMMatrixPerspectiveFovLH(30.0f, 1.0f, mFalloffStart, mFalloffEnd);
	XMMATRIX lightViewProj = lightView * lightProj;
	XMMATRIX shadowTransform = lightViewProj * toTextureTransform;

	XMStoreFloat4x4(&lightData.mWorld, XMMatrixTranspose(GetWorld()));
	XMStoreFloat4x4(&lightData.mViweProj, XMMatrixTranspose(lightViewProj));
	XMStoreFloat4x4(&lightData.mShadowTransform, XMMatrixTranspose(shadowTransform));
}