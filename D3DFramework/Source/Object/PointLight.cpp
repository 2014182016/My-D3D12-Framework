#include "pch.h"
#include "PointLight.h"
#include "Structure.h"

using namespace DirectX;

PointLight::PointLight(std::string&& name, ID3D12Device* device) : Light(std::move(name))
{
	mLightType = LightType::PointLight;
}

PointLight::~PointLight() { }

void PointLight::SetLightData(LightData& lightData)
{
	__super::SetLightData(lightData);
}