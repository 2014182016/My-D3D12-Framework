#include "WinApp.h"
#include <WindowsX.h>

WinApp* WinApp::mApp = nullptr;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return WinApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

WinApp* WinApp::GetApp()
{
	return mApp;
}

WinApp::WinApp(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName)
	: mhAppInst(hInstance),
	  mScreenWidth(screenWidth),
	  mScreenHeight(screenHeight),
	  mApplicationName(applicationName)
{
	// ���� �ϳ��� App���� �����Ѵ�.
	assert(mApp == nullptr);
	mApp = this;
}

WinApp::~WinApp()  { }

HINSTANCE WinApp::GetAppInst()const
{
	return mhAppInst;
}

HWND WinApp::GetMainWnd()const
{
	return mhMainWnd;
}

float WinApp::GetAspectRatio()const
{
	return static_cast<float>(mScreenWidth) / mScreenHeight;
}


int WinApp::Run()
{
	MSG msg = { 0 };

	mTimer.Reset();

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
			mTimer.Tick();
			float deltaTime = mTimer.GetDeltaTime();

			if (!mAppPaused)
			{
				CalculateFrameStats();
				Tick(deltaTime);
				Render();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool WinApp::Initialize()
{
	if (!InitMainWindow())
		return false;

	for (int i = 0; i < 256; ++i)
		mKeys[i] = false;

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
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
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
			// SetWindosPos�� SetFullscreenState�� �θ� ��
			else 
			{
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
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}

		OnKeyUp((unsigned int)wParam);

		return 0;
	case WM_KEYDOWN:
		OnKeyDown((unsigned int)wParam);
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
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mApplicationName.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
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
	if ((mTimer.GetTotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = mApplicationName +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

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
}

void WinApp::OnKeyDown(unsigned int input)
{
	mKeys[input] = true;
}

void WinApp::OnKeyUp(unsigned int input)
{
	mKeys[input] = false;
}