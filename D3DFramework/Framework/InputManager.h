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
};