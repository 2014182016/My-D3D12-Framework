#include <Object/DirectionalLight.h>
#include <Framework/SimpleShadowMap.h>

DirectionalLight::DirectionalLight(std::string&& name, ID3D12Device* device) : Light(std::move(name))
{
	lightType = LightType::DirectioanlLight;
	shadowMap = std::make_unique<SimpleShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	shadowMap->BuildResource(device);
}

DirectionalLight::~DirectionalLight() { }

void DirectionalLight::SetLightData(LightData& lightData)
{
	__super::SetLightData(lightData);

	// 그림자 맵에 사용하기 위한 행렬을 계산한다.
	XMVECTOR lightDir = XMLoadFloat3(&lightData.direction);
	XMVECTOR lightPos = XMLoadFloat3(&GetPosition());
	XMVECTOR targetPos = lightPos + lightDir * falloffEnd;
	XMVECTOR lightUp = XMLoadFloat3(&GetUp());
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);
	XMMATRIX lightProj = XMMatrixOrthographicLH(shadowMapSize.x, shadowMapSize.y, falloffStart, falloffEnd);

	XMMATRIX shadowTransform = lightView * lightProj * toTextureTransform;
	XMStoreFloat4x4(&lightData.shadowTransform, XMMatrixTranspose(shadowTransform));

	XMMATRIX invLightView = XMMatrixInverse(&XMMatrixDeterminant(lightView), lightView);
	DirectX::BoundingFrustum::CreateFromMatrix(lightFrustum, lightProj);
	lightFrustum.Transform(lightFrustum, invLightView);

	XMStoreFloat4x4(&view, lightView);
	XMStoreFloat4x4(&proj, lightProj);
}