#pragma once

#include "Light.h"

class DirectionalLight : public Light
{
public:
	DirectionalLight(std::string&& name, ID3D12Device* device);
	virtual ~DirectionalLight();

public:
	virtual void SetLightData(struct LightData& lightData) override;

};