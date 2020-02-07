#include "pch.h"
#include "Light.h"
#include "Structures.h"
#include "Interfaces.h"

using namespace DirectX;

Light::Light(std::string&& name) : Object(std::move(name)) { }

Light::~Light() { }

void Light::SetStrength(float r, float g, float b)
{
	mStrength.x = r;
	mStrength.y = g;
	mStrength.z = b;
}

void Light::SetShadowMapSize(float width, float height)
{
	mShadowMapSize.x = width;
	mShadowMapSize.y = height;
}

void Light::SetLightData(LightData& lightData)
{
	lightData.mPosition = GetPosition();
	lightData.mDirection = GetLook();
	lightData.mStrength = mStrength;
	lightData.mFalloffStart = mFalloffStart;
	lightData.mFalloffEnd = mFalloffEnd;
	lightData.mSpotAngle = mSpotAngle;
	lightData.mType = (std::uint32_t)mLightType;
	lightData.mEnabled = mEnabled;
}

void Light::RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList)
{
	mShadowMap->RenderSceneToShadowMap(cmdList, &mLightFrustum);
}