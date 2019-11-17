#pragma once

#include "pch.h"
#include "Light.h"

class SpotLight : public Light
{
public:
	SpotLight(std::string name);
	virtual ~SpotLight();

public:
	virtual void GetLightConstants(struct LightConstants& lightConstants) override;

public:
	inline float GetFalloffStart() const { return mFalloffStart; }
	inline void SetFalloffStart(float falloffStart) { mFalloffStart = falloffStart; }
	inline float GetFalloffEnd() const { return mFalloffEnd; }
	inline void SetFalloffEnd(float falloffEnd) { mFalloffEnd = falloffEnd; }
	inline float GetSpotPower() const { return mSpotPower; }
	inline void SetSpotPower(float spotPower) { mSpotPower = spotPower; }

protected:
	float mFalloffStart;
	float mFalloffEnd;
	float mSpotPower;
};