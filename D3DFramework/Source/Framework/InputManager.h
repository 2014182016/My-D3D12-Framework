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

public:
	static InputManager* GetInstance();

public:
	// ���� Ű�� �ƽ�Ű �ڵ��� ����ŭ �����Ѵ�.
	// ���� Ű�� ��� ���� ������ ���� �� �ִ�.
	static inline bool keys[256];
	// �������� ��ġ�� ���콺 ��ġ ��
	static inline POINT lastMousePos;
};