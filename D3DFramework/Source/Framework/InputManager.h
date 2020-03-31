#pragma once

#include <Windows.h>

/*
�÷��̾��� �Է¿� ���� �ൿ���� �����Ѵ�.
*/
class InputManager
{
public:
	InputManager();
	~InputManager();

public:
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

	virtual void OnKeyDown(unsigned int input);
	virtual void OnKeyUp(unsigned int input);

	virtual void Tick(float deltaTime);

public:
	static InputManager* GetInstance();

private:
	// ���� Ű�� �ƽ�Ű �ڵ��� ����ŭ �����Ѵ�.
	// ���� Ű�� ��� ���� ������ ���� �� �ִ�.
	bool keys[256];
	// �������� ��ġ�� ���콺 ��ġ ��
	POINT lastMousePos;

	float cameraWalkSpeed = 50.0f;
	float cameraRotateSpeed = 0.25f;
};