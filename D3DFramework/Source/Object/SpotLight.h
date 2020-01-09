#pragma once

#include "Light.h"

class SpotLight : public Light
{
public:
	SpotLight(std::string&& name, ID3D12Device* device);
	virtual ~SpotLight();

public:
	virtual void SetLightData(struct LightData& lightData) override;

};