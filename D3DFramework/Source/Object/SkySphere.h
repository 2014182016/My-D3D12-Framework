#pragma once

#include "GameObject.h"

/*
커다란 하늘로 주위를 도는 하늘 배경
*/
class SkySphere : public GameObject
{
public:
	SkySphere(std::string&& name);
	virtual ~SkySphere();

public:
	virtual void Tick(float deltaTime) override;

protected:
	// 배경이 돌아가는 속도
	float rotatingSpeed = 1.0f;
};