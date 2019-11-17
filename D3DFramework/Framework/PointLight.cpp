#include "pch.h"
#include "PointLight.h"
#include "Structures.h"

PointLight::PointLight(std::string name) : Light(name)
{
	mLightType = LightType::PointLight;
}

PointLight::~PointLight() { }

void PointLight::GetLightConstants(LightConstants& lightConstants)
{
	__super::GetLightConstants(lightConstants);

	lightConstants.mPosition = GetPosition();
	lightConstants.mFalloffStart = mFalloffStart;
	lightConstants.mFalloffEnd = mFalloffEnd;
}