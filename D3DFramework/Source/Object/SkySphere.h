#pragma once

#include "GameObject.h"

/*
Ŀ�ٶ� �ϴ÷� ������ ���� �ϴ� ���
*/
class SkySphere : public GameObject
{
public:
	SkySphere(std::string&& name);
	virtual ~SkySphere();

public:
	virtual void Tick(float deltaTime) override;

protected:
	// ����� ���ư��� �ӵ�
	float rotatingSpeed = 1.0f;
};