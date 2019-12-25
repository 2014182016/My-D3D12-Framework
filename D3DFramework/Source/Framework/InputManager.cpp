#include "pch.h"
#include "InputManager.h"
#include "D3DFramework.h"
#include "Camera.h"
#include "AssetManager.h"
#include "Sound.h"

InputManager::InputManager()
{
	for (int i = 0; i < 256; ++i)
		mKeys[i] = false;
}

InputManager::~InputManager() { }

void InputManager::OnMouseDown(WPARAM btnState, int x, int y) 
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		// ���콺�� ������ �ʰ� �Ѵ�.
		::SetCursor(NULL);

		SetCapture(D3DFramework::GetApp()->GetMainWnd());
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		D3DFramework::GetInstance()->Picking(x, y);
	}
}

void InputManager::OnMouseUp(WPARAM btnState, int x, int y) 
{
	ReleaseCapture();
}

void InputManager::OnMouseMove(WPARAM btnState, int x, int y) 
{
	Camera* camera = D3DFramework::GetInstance()->GetCamera();

	// ���� ���콺 ��ư�� �������ٸ� ī�޶� ȸ����Ų��.
	if ((btnState & MK_LBUTTON) != 0)
	{
		// 1�ȼ� �����ӿ� mCameraRotateSpeed��ŭ ȸ���Ѵ�.
		float dx = DirectX::XMConvertToRadians(mCameraRotateSpeed * static_cast<float>(x - mLastMousePos.x));
		float dy = DirectX::XMConvertToRadians(mCameraRotateSpeed * static_cast<float>(y - mLastMousePos.y));

		// ȸ���� ������ �����̴�.
		camera->RotateY(dx);
		camera->Pitch(dy);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void InputManager::OnKeyDown(unsigned int input)
{
	// input�� �׻� ���ڸ� ������ �ʴ´ٴ� ���� �����Ѵ�.
	mKeys[input] = true;
}

void InputManager::OnKeyUp(unsigned int input)
{
	mKeys[input] = false;

	if (input == VK_F1)
		D3DFramework::GetApp()->SwitchOptionEnabled(Option::Wireframe);
	else if (input == VK_F2)
		D3DFramework::GetApp()->SwitchOptionEnabled(Option::Debug_Collision);
	else if (input == VK_F3)
		D3DFramework::GetApp()->SwitchOptionEnabled(Option::Debug_Octree);
	else if (input == VK_F4)
		D3DFramework::GetApp()->SwitchOptionEnabled(Option::Debug_Light);
}

void InputManager::Tick(float deltaTime) 
{
	Camera* camera = D3DFramework::GetInstance()->GetCamera();

	if (mKeys['w'] || mKeys['W'])
		camera->Walk(mCameraWalkSpeed * deltaTime);

	if (mKeys['s'] || mKeys['S'])
		camera->Walk(-mCameraWalkSpeed * deltaTime);

	if (mKeys['a'] || mKeys['A'])
		camera->Strafe(-mCameraWalkSpeed * deltaTime);

	if (mKeys['d'] || mKeys['D'])
		camera->Strafe(mCameraWalkSpeed * deltaTime);

	camera->UpdateViewMatrix();
}