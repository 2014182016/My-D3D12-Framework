#pragma once

#include "Light.h"

class PointLight : public Light
{
public:
	PointLight(std::string name);
	~PointLight();

public:
	virtual void GetLightConstants(struct LightConstants& lightConstants) override;

public:
	inline float GetFalloffStart() const { return mFalloffStart; }
	inline void SetFalloffStart(float falloffStart) { mFalloffStart = falloffStart; }
	inline float GetFalloffEnd() const { return mFalloffEnd; }
	inline void SetFalloffEnd(float falloffEnd) { mFalloffEnd = falloffEnd; }

protected:
	float mFalloffStart;
	float mFalloffEnd;
};