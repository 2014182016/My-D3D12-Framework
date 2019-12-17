#pragma once

#include "Light.h"

class SpotLight : public Light
{
public:
	SpotLight(std::string&& name);
	virtual ~SpotLight();

public:
	virtual void SetLightData(struct LightData& lightData, const DirectX::BoundingSphere& sceneBounding) override;

public:
	inline float GetFalloffStart() const { return mFalloffStart; }
	inline void SetFalloffStart(float falloffStart) { mFalloffStart = falloffStart; }
	inline float GetFalloffEnd() const { return mFalloffEnd; }
	inline void SetFalloffEnd(float falloffEnd) { mFalloffEnd = falloffEnd; }
	inline float GetSpotPower() const { return mSpotPower; }
	inline void SetSpotPower(float spotPower) { mSpotPower = spotPower; }

protected:
	float mFalloffStart = 1.0f;
	float mFalloffEnd = 10.0f;
	float mSpotPower = 1.0f;
};