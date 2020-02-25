#pragma once

#include "GameObject.h"

class SkySphere : public GameObject
{
public:
	SkySphere(std::string&& name);
	virtual ~SkySphere();

public:
	virtual void Tick(float deltaTime) override;

protected:
	float rotatingSpeed = 1.0f;

};