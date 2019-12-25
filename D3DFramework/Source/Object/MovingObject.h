#pragma once

#include "GameObject.h"

class MovingObject : public GameObject
{
public:
	MovingObject(std::string&& name);
	virtual ~MovingObject();

public:
	virtual void Tick(float deltaTime) override;
};

