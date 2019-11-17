#include "pch.h"
#include "Light.h"
#include "Structures.h"

Light::Light(std::string name) : Object(name) { }

Light::~Light() { }


void Light::SetStrength(float r, float g, float b)
{
	mStrength.x = r;
	mStrength.y = g;
	mStrength.z = b;
}

void Light::GetLightConstants(LightConstants& lightConstants)
{
	lightConstants.mStrength = mStrength;
	lightConstants.mType = (std::uint32_t)mLightType;
	lightConstants.mEnabled = mEnabled;
}