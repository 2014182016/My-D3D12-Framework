#include "../PrecompiledHeader/pch.h"
#include "InputManager.h"
#include "D3DFramework.h"
#include "Camera.h"
#include "Physics.h"
#include "D3DDebug.h"

InputManager::InputManager()
{
	for (int i = 0; i < 256; ++i)
		keys[i] = false;
}

InputManager::~InputManager() { }

InputManager* InputManager::GetInstance()
{
	static InputManager* instance = nullptr;
	if (instance == nullptr)
		instance = new InputManager();
	return instance;
}

void InputManager::OnMouseDown(WPARAM btnState, int x, int y) 
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		lastMousePos.x = x;
		lastMousePos.y = y;

		// 마우스를 보이지 않게 한다.
		::SetCursor(NULL);

		SetCapture(D3DFramework::GetInstance()->GetMainWnd());
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		HitInfo hitInfo;
		bool result = D3DFramework::GetInstance()->Picking(hitInfo, x, y);
	}
}

void InputManager::OnMouseUp(WPARAM btnState, int x, int y) 
{
	ReleaseCapture();
}

void InputManager::OnMouseMove(WPARAM btnState, int x, int y) 
{
	Camera* camera = D3DFramework::GetInstance()->GetCamera();

	// 왼쪽 마우스 버튼이 눌려졌다면 카메라를 회전시킨다.
	if ((btnState & MK_LBUTTON) != 0)
	{
		// 1픽셀 움직임에 cameraRotateSpeed만큼 회전한다.
		float dx = DirectX::XMConvertToRadians(cameraRotateSpeed * static_cast<float>(x - lastMousePos.x));
		float dy = DirectX::XMConvertToRadians(cameraRotateSpeed * static_cast<float>(y - lastMousePos.y));

		// 회전의 단위는 라디안이다.
		camera->RotateY(dx);
		camera->Pitch(dy);
	}

	lastMousePos.x = x;
	lastMousePos.y = y;
}

void InputManager::OnKeyDown(unsigned int input)
{
	// input이 항상 문자만 들어오지 않는다는 것을 주의한다.
	keys[input] = true;
}

void InputManager::OnKeyUp(unsigned int input)
{
	keys[input] = false;

#if defined(DEBUG) || defined(_DEBUG)
	if (input == VK_ESCAPE)
		DestroyWindow(D3DFramework::GetInstance()->GetMainWnd());
	else if (input == VK_F1)
		D3DFramework::GetInstance()->SwitchOptionEnabled(Option::Wireframe);
	else if (input == VK_F2)
		D3DFramework::GetInstance()->SwitchOptionEnabled(Option::Debug_GBuffer);
	else if (input == VK_F5)
		D3DFramework::GetInstance()->DrawDebugCollision();
	else if (input == VK_F6)
		D3DFramework::GetInstance()->DrawDebugLight();
	else if (input == VK_F7)
		D3DFramework::GetInstance()->DrawDebugOctree();
	else if (input == VK_F8)
		D3DDebug::GetInstance()->Clear();
#endif
}

void InputManager::Tick(float deltaTime) 
{
	Camera* camera = D3DFramework::GetInstance()->GetCamera();

	if (keys['w'] || keys['W'])
		camera->Walk(cameraWalkSpeed * deltaTime);

	if (keys['s'] || keys['S'])
		camera->Walk(-cameraWalkSpeed * deltaTime);

	if (keys['a'] || keys['A'])
		camera->Strafe(-cameraWalkSpeed * deltaTime);

	if (keys['d'] || keys['D'])
		camera->Strafe(cameraWalkSpeed * deltaTime);

	camera->UpdateViewMatrix();
}