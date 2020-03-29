#include <Object/PointLight.h>

PointLight::PointLight(std::string&& name, ID3D12Device* device) : Light(std::move(name))
{
	lightType = LightType::PointLight;
}

PointLight::~PointLight() { }

void PointLight::SetLightData(LightData& lightData)
{
	__super::SetLightData(lightData);
}