#include "pch.h"
#include "WinApp.h"
#include "InputManager.h"
#include "GameTimer.h"
#include <WindowsX.h>

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (WinApp::GetApp() == nullptr)
		return 0;
	return WinApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

WinApp::WinApp(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName)
	: mhAppInst(hInstance),
	  mScreenWidth(screenWidth),
	  mScreenHeight(screenHeight),
	  mApplicationName(applicationName)
{
	assert(app == nullptr);
	app = this;

	mGameTimer = std::make_unique<GameTimer>();
	mInputManager = std::make_unique<InputManager>();
	mOptions.reset();
}

WinApp::~WinApp() 
{ 
	app = nullptr;

	mGameTimer = nullptr;
	mInputManager = nullptr;
}


int WinApp::Run()
{
	MSG msg = { 0 };

	mGameTimer->Reset();

	while (msg.message != WM_QUIT)
	{
		// ������ �޽����� �ִٸ� �װ͵��� ó���Ѵ�.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// �ƴϸ� ������ �����Ѵ�.
		else
		{
			mGameTimer->Tick();
			float deltaTime = mGameTimer->GetDeltaTime();

			if (!mAppPaused)
			{
				CalculateFrameStats();
				mInputManager->Tick(deltaTime);
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
	if (!InitMainWindow())
		return false;

	// pdh�� �ʱ�ȭ�Ѵ�.
	PdhOpenQuery(NULL, NULL, &mQueryHandle);
	PdhAddCounter(mQueryHandle, L"\\Processor(_Total)\\% Processor Time", NULL, &mCounterHandle);
	PdhCollectQueryData(mQueryHandle);

	return true;
}


LRESULT WinApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mGameTimer->Stop();
		}
		else
		{
			mAppPaused = false;
			mGameTimer->Start();
		}
		return 0;
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			mAppPaused = true;
			mMinimized = true;
			mMaximized = false;
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			mAppPaused = false;
			mMinimized = false;
			mMaximized = true;
			OnResize(mScreenWidth, mScreenHeight);
		}
		else if (wParam == SIZE_RESTORED)
		{
			if (mMinimized)
			{
				mAppPaused = false;
				mMinimized = false;
				OnResize(mScreenWidth, mScreenHeight);
			}
			else if (mMaximized)
			{
				mAppPaused = false;
				mMaximized = false;
				OnResize(mScreenWidth, mScreenHeight);
			}
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MENUCHAR:
		// Alt-Enter�� ���� ��, ��- �Ҹ��� ���� �ʰ� �Ѵ�.
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if(mInputManager.get())
			mInputManager->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		if (mInputManager.get())
			mInputManager->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		if (mInputManager.get())
			mInputManager->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (mInputManager.get())
			mInputManager->OnKeyUp((unsigned int)wParam);
		return 0;
	case WM_KEYDOWN:
		if (mInputManager.get())
			mInputManager->OnKeyDown((unsigned int)wParam);
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
	wc.hInstance = mhAppInst;
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

	// ������ â���� Ŭ���̾�Ʈ ������ ����Ѵ�.
	RECT R = { 0, 0, mScreenWidth, mScreenHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mApplicationName.c_str(),
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}


void WinApp::CalculateFrameStats()
{
	// 1�ʿ� ��� �����Ӱ� �� �����ӿ� �׸��� �ð��� ����ϰ�,
	// Window Caption�� �̸� ǥ���Ѵ�.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// 1�ʰ� �Ѿ�� ����Ѵ�.
	if ((mGameTimer->GetTotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		// cpu ��뷮�� ����Ѵ�.
		PDH_FMT_COUNTERVALUE counterValue;
		PdhCollectQueryData(mQueryHandle);
		PdhGetFormattedCounterValue(mCounterHandle, PDH_FMT_LONG, NULL, &counterValue);

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		std::wstring cpuStr = std::to_wstring(counterValue.longValue);

		std::wstring windowText = mApplicationName +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr +
			L"    cpu: " + cpuStr + L"%";

		SetWindowText(mhMainWnd, windowText.c_str());

		// ���� ����� ����ϱ� ���� �����Ѵ�.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void WinApp::OnResize(int screenWidth, int screenHeight)
{
	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;

	// ������ â���� Ŭ���̾�Ʈ ������ ����Ѵ�.
	RECT R = { 0, 0, mScreenWidth, mScreenHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	SetWindowPos(mhMainWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
}

void WinApp::SetOptionEnabled(Option option, bool value)
{
	mOptions.set((int)option, value); 

	ApplyOption(option);
}

void WinApp::SwitchOptionEnabled(Option option)
{
	mOptions.flip((int)option);

	ApplyOption(option);
}