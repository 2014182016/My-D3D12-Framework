#include "../PrecompiledHeader/pch.h"
#include "SkySphere.h"

SkySphere::SkySphere(std::string&& name) : GameObject(std::move(name)) { }

SkySphere::~SkySphere() { }

void SkySphere::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	float speed = rotatingSpeed * deltaTime;
	Rotate(0.0f, speed, 0.0f);
}