#pragma once

#include "Light.h"

class DirectionalLight : public Light
{
public:
	DirectionalLight(std::string&& name);
	virtual ~DirectionalLight();

public:
	virtual void SetLightData(struct LightData& lightData, const DirectX::BoundingSphere& sceneBounding) override;

};