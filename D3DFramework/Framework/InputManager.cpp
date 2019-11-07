#include "pch.h"
#include "InputManager.h"
#include "D3DFramework.h"
#include "Camera.h"

InputManager::InputManager()
{
	for (int i = 0; i < 256; ++i)
		mKeys[i] = false;

	mApp = D3DFramework::GetApp();
	assert(mApp);
}

InputManager::~InputManager() { }

void InputManager::OnMouseDown(WPARAM btnState, int x, int y) 
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	// ���콺�� ������ �ʰ� �Ѵ�.
	::SetCursor(NULL);

	SetCapture(mApp->GetMainWnd());
}

void InputManager::OnMouseUp(WPARAM btnState, int x, int y) 
{
	ReleaseCapture();
}

void InputManager::OnMouseMove(WPARAM btnState, int x, int y) 
{
	// ���� ���콺 ��ư�� �������ٸ� ī�޶� ȸ����Ų��.
	if ((btnState & MK_LBUTTON) != 0)
	{
		// 1�ȼ� �����ӿ� mCameraRotateSpeed��ŭ ȸ���Ѵ�.
		float dx = DirectX::XMConvertToRadians(mCameraRotateSpeed * static_cast<float>(x - mLastMousePos.x));
		float dy = DirectX::XMConvertToRadians(mCameraRotateSpeed * static_cast<float>(y - mLastMousePos.y));

		// ȸ���� ������ �����̴�.
		mApp->GetCamera()->RotateY(dx);
		mApp->GetCamera()->Pitch(dy);
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
		mApp->SetIsWireframe(!mApp->GetIsWireframe());
	else if (input == VK_F4)
		mApp->SetFullscreenState(!mApp->GetFullscreenState());
	else if (input == VK_ESCAPE)
		mApp->OnDestroy();
}

void InputManager::Tick(float deltaTime) 
{
	if (mKeys['w'] || mKeys['W'])
		mApp->GetCamera()->Walk(mCameraWalkSpeed * deltaTime);

	if (mKeys['s'] || mKeys['S'])
		mApp->GetCamera()->Walk(-mCameraWalkSpeed * deltaTime);

	if (mKeys['a'] || mKeys['A'])
		mApp->GetCamera()->Strafe(-mCameraWalkSpeed * deltaTime);

	if (mKeys['d'] || mKeys['D'])
		mApp->GetCamera()->Strafe(mCameraWalkSpeed * deltaTime);

	mApp->GetCamera()->UpdateViewMatrix();
}