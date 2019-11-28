#include "pch.h"
#include "MovingObject.h"

MovingObject::MovingObject(std::string name) : GameObject(name) { }


MovingObject::~MovingObject() { }

void MovingObject::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	AddForce(0.0f, 75.0f, 0.0f);
}
