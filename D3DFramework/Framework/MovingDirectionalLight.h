#pragma once

#include "DirectionalLight.h"

class MovingDirectionalLight : public DirectionalLight
{
public:
	MovingDirectionalLight(std::string&& name);
	virtual ~MovingDirectionalLight();

public:
	virtual void Tick(float deltaTime) override;
};