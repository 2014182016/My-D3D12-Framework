#include <Object/Light.h>
#include <Framework/ShadowMap.h>

Light::Light(std::string&& name) : Object(std::move(name)) { }

Light::~Light() { }

void Light::SetLightData(LightData& lightData)
{
	lightData.position = position;
	lightData.direction = GetLook();
	lightData.strength = strength;
	lightData.falloffStart = falloffStart;
	lightData.falloffEnd = falloffEnd;
	lightData.spotAngle = spotAngle;
	lightData.type = (UINT32)lightType;
	lightData.enabled = enabled;
}

void Light::RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList)
{
	shadowMap->RenderSceneToShadowMap(cmdList, &lightFrustum);
}

ShadowMap* Light::GetShadowMap()
{
	return shadowMap.get();
}

XMMATRIX Light::GetView() const
{
	return XMLoadFloat4x4(&view);
}

XMMATRIX  Light::GetProj() const
{
	return XMLoadFloat4x4(&proj);
}