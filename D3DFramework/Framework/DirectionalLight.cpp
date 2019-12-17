#include "pch.h"
#include "DirectionalLight.h"
#include "Structures.h"

using namespace DirectX;

DirectionalLight::DirectionalLight(std::string&& name) : Light(std::move(name))
{
	mLightType = LightType::DirectioanlLight;
}

DirectionalLight::~DirectionalLight() { }

void DirectionalLight::SetLightData(LightData& lightData, const BoundingSphere& sceneBounding)
{
	// NDC ���� [-1, 1]^2�� �ؽ�ó ���� [0, 1]^2���� ��ȯ�ϴ� ���
	static XMMATRIX toTextureTransform(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	__super::SetLightData(lightData, sceneBounding);

	lightData.mDirection = GetLook();

	// �׸��� �ʿ� ����ϱ� ���� ����� ����Ѵ�.
	XMVECTOR lightDir = XMLoadFloat3(&GetLook());
	XMVECTOR lightPos = -2.0f * sceneBounding.Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&sceneBounding.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));
	float l = sphereCenterLS.x - sceneBounding.Radius;
	float b = sphereCenterLS.y - sceneBounding.Radius;
	float n = sphereCenterLS.z - sceneBounding.Radius;
	float r = sphereCenterLS.x + sceneBounding.Radius;
	float t = sphereCenterLS.y + sceneBounding.Radius;
	float f = sphereCenterLS.z + sceneBounding.Radius;

	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
	XMMATRIX lightViewProj = lightView * lightProj;
	XMMATRIX shadowTransform = lightViewProj * toTextureTransform;

	XMStoreFloat4x4(&lightData.mWorld, XMMatrixTranspose(GetWorld()));
	XMStoreFloat4x4(&lightData.mViweProj, XMMatrixTranspose(lightViewProj));
	XMStoreFloat4x4(&lightData.mShadowTransform, XMMatrixTranspose(shadowTransform));
}