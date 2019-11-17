#include "pch.h"
#include "SpotLight.h"
#include "Structures.h"

SpotLight::SpotLight(std::string name) : Light(name)
{
	mLightType = LightType::SpotLight;
}

SpotLight::~SpotLight() { }

void SpotLight::GetLightConstants(LightConstants& lightConstants)
{
	__super::GetLightConstants(lightConstants);

	lightConstants.mPosition = GetPosition();
	lightConstants.mDirection = GetLook();
	lightConstants.mFalloffStart = mFalloffStart;
	lightConstants.mFalloffEnd = mFalloffEnd;
	lightConstants.mSpotPower = mSpotPower;
}