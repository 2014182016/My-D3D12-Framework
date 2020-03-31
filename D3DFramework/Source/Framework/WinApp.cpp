#include "../PrecompiledHeader/pch.h"
#include "WinApp.h"
#include "InputManager.h"
#include "GameTimer.h"
#include <WindowsX.h>

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (WinApp::GetInstance() == nullptr)
		return 0;
	return WinApp::GetInstance()->MsgProc(hwnd, msg, wParam, lParam);
}

WinApp::WinApp(HINSTANCE hInstance, const INT32 screenWidth, const INT32 screenHeight, 
	const std::wstring applicationName, const bool useWinApi)
	: hAppInst(hInstance),
	  screenWidth(screenWidth),
	  screenHeight(screenHeight),
	  applicationName(applicationName),
	  useWinApi(useWinApi)
{
	instance = this;

	gameTimer = std::make_unique<GameTimer>();
}

WinApp::~WinApp() 
{ 
	gameTimer = nullptr;
}

WinApp* WinApp::GetInstance()
{
	return instance;
}

int WinApp::Run()
{
	MSG msg = { 0 };

	gameTimer->Reset();

	while (msg.message != WM_QUIT)
	{
		// 윈도우 메시지가 있다면 그것들을 처리한다.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// 아니면 게임을 진행한다.
		else
		{
			gameTimer->Tick();
			float deltaTime = gameTimer->GetDeltaTime();

			if (!appPaused)
			{
				CalculateFrameStats();
				InputManager::GetInstance()->Tick(deltaTime);
				Tick(deltaTime);
				Render();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	OnDestroy();

	return (int)msg.wParam;
}

bool WinApp::Initialize()
{
	if (useWinApi)
	{
		if (!InitMainWindow())
			return false;
	}

	// pdh를 초기화한다.
	PdhOpenQuery(NULL, NULL, &queryHandle);
	PdhAddCounter(queryHandle, L"\\Processor(_Total)\\% Processor Time", NULL, &counterHandle);
	PdhCollectQueryData(queryHandle);

	return true;
}


LRESULT WinApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			appPaused = true;
			gameTimer->Stop();
		}
		else
		{
			appPaused = false;
			gameTimer->Start();
		}
		return 0;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			appPaused = true;
			minimized = true;
			maximized = false;
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			appPaused = false;
			minimized = false;
			maximized = true;
			OnResize(screenWidth, screenHeight);
		}
		else if (wParam == SIZE_RESTORED)
		{
			if (minimized)
			{
				appPaused = false;
				minimized = false;
				OnResize(screenWidth, screenHeight);
			}
			else if (maximized)
			{
				appPaused = false;
				maximized = false;
				OnResize(screenWidth, screenHeight);
			}
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MENUCHAR:
		// Alt-Enter를 누를 때, 삐- 소리가 나지 않게 한다.
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		InputManager::GetInstance()->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		InputManager::GetInstance()->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		InputManager::GetInstance()->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		InputManager::GetInstance()->OnKeyUp((unsigned int)wParam);
		return 0;
	case WM_KEYDOWN:
		InputManager::GetInstance()->OnKeyDown((unsigned int)wParam);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool WinApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// 윈도우 창에서 클라이언트 영역을 계산한다.
	RECT R = { 0, 0, screenWidth, screenHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	hMainWnd = CreateWindow(L"MainWnd", applicationName.c_str(),
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hAppInst, 0);
	if (!hMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(hMainWnd, SW_SHOW);
	UpdateWindow(hMainWnd);

	return true;
}


void WinApp::CalculateFrameStats()
{
	// 1초에 평균 프레임과 한 프레임에 그리는 시간을 계산하고,
	// Window Caption에 이를 표시한다.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// 1초가 넘어가면 계산한다.
	if ((gameTimer->GetTotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		// cpu 사용량을 계산한다.
		PDH_FMT_COUNTERVALUE counterValue;
		PdhCollectQueryData(queryHandle);
		PdhGetFormattedCounterValue(counterHandle, PDH_FMT_LONG, NULL, &counterValue);

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		std::wstring cpuStr = std::to_wstring(counterValue.longValue);

		std::wstring windowText = applicationName +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr +
			L"    cpu: " + cpuStr + L"%";

		SetWindowText(hMainWnd, windowText.c_str());

		// 다음 평균을 계산하기 위해 리셋한다.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void WinApp::OnResize(const INT32 screenWidth, const INT32 screenHeight)
{
	this->screenWidth = screenWidth;
	this->screenHeight = screenHeight;

	if (useWinApi)
	{
		// 윈도우 창에서 클라이언트 영역을 계산한다.
		RECT R = { 0, 0, screenWidth, screenHeight };
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, false);
		int width = R.right - R.left;
		int height = R.bottom - R.top;

		SetWindowPos(hMainWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
	}
}

float WinApp::GetAspectRatio() const
{
	return static_cast<float>(screenWidth) / screenHeight; 
}

HINSTANCE WinApp::GetAppInst() const
{
	return hAppInst; 
}

void WinApp::SetMainWnd(HWND hwnd)
{
	hMainWnd = hwnd;
}

HWND WinApp::GetMainWnd() const
{
	return hMainWnd; 
}

INT32 WinApp::GetScreenWidth() const
{
	return screenWidth; 
}
INT32 WinApp::GetScreenHeight() const
{
	return screenHeight; 
}

GameTimer* WinApp::GetGameTimer()
{
	return gameTimer.get();
}