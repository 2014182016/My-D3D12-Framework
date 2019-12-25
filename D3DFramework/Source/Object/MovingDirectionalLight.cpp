#include "pch.h"
#include "MovingDirectionalLight.h"

MovingDirectionalLight::MovingDirectionalLight(std::string&& name) : DirectionalLight(std::move(name)) { }

MovingDirectionalLight::~MovingDirectionalLight() { }

void MovingDirectionalLight::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	Rotate(0.0f, 45.0f * deltaTime, 0.0f);
}