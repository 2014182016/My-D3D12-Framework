#include "pch.h"
#include "DirectionalLight.h"
#include "Structures.h"

DirectionalLight::DirectionalLight(std::string name) : Light(name) 
{
	mLightType = LightType::DirectioanlLight;
}

DirectionalLight::~DirectionalLight() { }

void DirectionalLight::GetLightConstants(LightConstants& lightConstants)
{
	__super::GetLightConstants(lightConstants);

	lightConstants.mDirection = GetLook();
}