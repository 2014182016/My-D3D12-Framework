#pragma once

#include "pch.h"

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
	inline bool GetIsKeyDown(unsigned int input) const { return mKeys[input]; }
	inline POINT GetLastMoustPos() const { return mLastMousePos; }

private:
	bool mKeys[256];
	POINT mLastMousePos;

	float mCameraWalkSpeed = 50.0f;
	float mCameraRotateSpeed = 0.25f;

	// 프레임워크에서 원하는 함수 및 데이터를 사용하기 위한 App변수
	class D3DFramework* mApp = nullptr;
};