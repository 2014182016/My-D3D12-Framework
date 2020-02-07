#include "pch.h"
#include "DirectionalLight.h"
#include "Structures.h"
#include "SimpleShadowMap.h"

using namespace DirectX;

DirectionalLight::DirectionalLight(std::string&& name, ID3D12Device* device) : Light(std::move(name))
{
	mLightType = LightType::DirectioanlLight;
	mShadowMap = std::make_unique<SimpleShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	mShadowMap->BuildResource(device);
}

DirectionalLight::~DirectionalLight() { }

void DirectionalLight::SetLightData(LightData& lightData)
{

	__super::SetLightData(lightData);

	// 그림자 맵에 사용하기 위한 행렬을 계산한다.
	XMVECTOR lightDir = XMLoadFloat3(&lightData.mDirection);
	XMVECTOR lightPos = XMLoadFloat3(&GetPosition());
	XMVECTOR targetPos = lightPos + lightDir * mFalloffEnd;
	XMVECTOR lightUp = XMLoadFloat3(&GetUp());
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);
	XMMATRIX lightProj = XMMatrixOrthographicLH(mShadowMapSize.x, mShadowMapSize.y, 0.5f, mFalloffEnd);

	XMMATRIX shadowTransform = lightView * lightProj * toTextureTransform;
	XMStoreFloat4x4(&lightData.mShadowTransform, XMMatrixTranspose(shadowTransform));

	XMMATRIX invLightView = XMMatrixInverse(&XMMatrixDeterminant(lightView), lightView);
	DirectX::BoundingFrustum::CreateFromMatrix(mLightFrustum, lightProj);
	mLightFrustum.Transform(mLightFrustum, invLightView);

	XMStoreFloat4x4(&mView, lightView);
	XMStoreFloat4x4(&mProj, lightProj);
}