#pragma once

#include "Light.h"

class PointLight : public Light
{
public:
	PointLight(std::string&& name);
	~PointLight();

public:
	virtual void SetLightData(struct LightData& lightData, const DirectX::BoundingSphere& sceneBounding) override;

public:
	inline float GetFalloffStart() const { return mFalloffStart; }
	inline void SetFalloffStart(float falloffStart) { mFalloffStart = falloffStart; }
	inline float GetFalloffEnd() const { return mFalloffEnd; }
	inline void SetFalloffEnd(float falloffEnd) { mFalloffEnd = falloffEnd; }

protected:
	float mFalloffStart = 1.0f;
	float mFalloffEnd = 10.0f;
};