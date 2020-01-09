#include "pch.h"
#include "SpotLight.h"
#include "Structures.h"
#include "SimpleShadowMap.h"

using namespace DirectX;

SpotLight::SpotLight(std::string&& name, ID3D12Device* device) : Light(std::move(name))
{
	mLightType = LightType::SpotLight;
	mShadowMap = std::make_unique<SimpleShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	mShadowMap->BuildResource(device);
}

SpotLight::~SpotLight() { }

void SpotLight::SetLightData(LightData& lightData)
{
	// NDC 공간 [-1, 1]^2을 텍스처 공간 [0, 1]^2으로 변환하는 행렬
	static XMMATRIX toTextureTransform(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	__super::SetLightData(lightData);

	// 그림자 맵에 사용하기 위한 행렬을 계산한다.
	XMVECTOR lightDir = XMLoadFloat3(&lightData.mDirection);
	XMVECTOR lightPos = XMLoadFloat3(&GetPosition());
	XMVECTOR targetPos = lightPos + lightDir * mFalloffEnd;
	XMVECTOR lightUp = XMLoadFloat3(&GetUp());
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);
	XMMATRIX lightProj = XMMatrixOrthographicLH(mShadowMapSize.x, mShadowMapSize.y, 0.5f, mFalloffEnd);

	XMStoreFloat4x4(&mView, lightView);
	XMStoreFloat4x4(&mProj, lightProj);

	XMMATRIX shadowTransform = lightView * lightProj * toTextureTransform;
	XMStoreFloat4x4(&lightData.mShadowTransform, XMMatrixTranspose(shadowTransform));

	XMMATRIX invLightView = XMMatrixInverse(&XMMatrixDeterminant(lightView), lightView);
	BoundingFrustum lightFrustum;
	DirectX::BoundingFrustum::CreateFromMatrix(lightFrustum, lightProj);
	lightFrustum.Transform(lightFrustum, invLightView);
	mShadowMap->SetFrustum(lightFrustum);
}