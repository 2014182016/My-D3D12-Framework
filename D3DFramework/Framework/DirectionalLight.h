#pragma once

#include "pch.h"
#include "Light.h"

class DirectionalLight : public Light
{
public:
	DirectionalLight(std::string name);
	virtual ~DirectionalLight();

public:
	virtual void GetLightConstants(struct LightConstants& lightConstants) override;

};