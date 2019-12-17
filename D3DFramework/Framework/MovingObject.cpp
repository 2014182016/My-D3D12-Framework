#include "pch.h"
#include "MovingObject.h"

MovingObject::MovingObject(std::string&& name) : GameObject(std::move(name)) { }


MovingObject::~MovingObject() { }

void MovingObject::Tick(float deltaTime)
{
	__super::Tick(deltaTime);
}
