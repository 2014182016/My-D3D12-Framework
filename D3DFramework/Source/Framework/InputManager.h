#pragma once

#include <Windows.h>

/*
플레이어의 입력에 관한 행동들을 관리한다.
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
	// 눌린 키를 아스키 코드의 수만큼 보관한다.
	// 따라서 키의 모든 값을 가지고 있을 수 있다.
	bool keys[256];
	// 마지막에 위치한 마우스 위치 값
	POINT lastMousePos;

	float cameraWalkSpeed = 50.0f;
	float cameraRotateSpeed = 0.25f;
};