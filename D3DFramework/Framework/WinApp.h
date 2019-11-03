#pragma once

#include "pch.h"
#include "GameTimer.h"

class WinApp
{
public:
	WinApp(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName);
	WinApp(const WinApp& rhs) = delete;
	WinApp& operator=(const WinApp& rhs) = delete;
	virtual ~WinApp();

public:
	static WinApp* GetApp();

	HINSTANCE GetAppInst()const;
	HWND GetMainWnd()const;
	float GetAspectRatio()const;

	int Run();

	virtual bool Initialize();
	virtual void OnDestroy() { };

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void OnResize(int screenWidth, int screenHeight);
	virtual void Tick(float deltaTime) { };
	virtual void Render() { };

	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

	virtual void OnKeyDown(unsigned int input);
	virtual void OnKeyUp(unsigned int input);

private:
	bool InitMainWindow();
	void CalculateFrameStats();

protected:
	static WinApp* mApp;

	HINSTANCE mhAppInst = nullptr; 
	HWND mhMainWnd = nullptr; 

	// 응용 프로그램 상태
	bool mAppPaused = false; 
	bool mMinimized = false; 
	bool mMaximized = false; 
	bool mFullscreenState = false;

	// 키가 눌린 상태를 저장한다.
	bool mKeys[256];

	// 윈도우 속성
	std::wstring mApplicationName = L"D3DFramework";
	int mScreenWidth = 800;
	int mScreenHeight = 600;

	// 시간을 관리하는 타이머
	GameTimer mTimer;
};

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);