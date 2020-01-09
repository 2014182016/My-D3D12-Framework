#pragma once

#include "Light.h"

class PointLight : public Light
{
public:
	PointLight(std::string&& name, ID3D12Device* device);
	~PointLight();

public:
	virtual void SetLightData(struct LightData& lightData) override;

};