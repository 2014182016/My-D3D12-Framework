#include "pch.h"
#include "Light.h"
#include "Structures.h"

using namespace DirectX;

Light::Light(std::string&& name) : Object(std::move(name)) { }

Light::~Light() { }


void Light::SetStrength(float r, float g, float b)
{
	mStrength.x = r;
	mStrength.y = g;
	mStrength.z = b;
}

void Light::SetLightData(LightData& lightData, const BoundingSphere& sceneBounding)
{
	lightData.mPosition = GetPosition();
	lightData.mStrength = mStrength;
	lightData.mType = (std::uint32_t)mLightType;
	lightData.mEnabled = mEnabled;
}