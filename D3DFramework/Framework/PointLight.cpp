#include "pch.h"
#include "PointLight.h"
#include "Structures.h"

using namespace DirectX;

PointLight::PointLight(std::string&& name) : Light(std::move(name))
{
	mLightType = LightType::PointLight;
}

PointLight::~PointLight() { }

void PointLight::SetLightData(LightData& lightData, const DirectX::BoundingSphere& sceneBounding)
{
	__super::SetLightData(lightData, sceneBounding);

	lightData.mFalloffStart = mFalloffStart;
	lightData.mFalloffEnd = mFalloffEnd;
}