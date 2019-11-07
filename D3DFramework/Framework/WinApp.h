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
	virtual bool Initialize();
	virtual void OnDestroy();

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	int Run();

protected:
	virtual void OnResize(int screenWidth, int screenHeight);
	virtual void Tick(float deltaTime) { };
	virtual void Render() { };

public:
	inline static WinApp* GetApp() { return mApp; }

	inline float GetAspectRatio() const { return static_cast<float>(mScreenWidth) / mScreenHeight; }
	inline HINSTANCE GetAppInst() const { return mhAppInst; }
	inline HWND GetMainWnd() const { return mhMainWnd; }

	inline bool GetFullscreenState() const { return mFullscreenState; }

private:
	bool InitMainWindow();
	void CalculateFrameStats();

protected:
	// �ܺο��� �����ϱ� ���� �̱��� ����� Ŭ���� �ν��Ͻ�
	static WinApp* mApp;

	// ���� ���α׷� ����
	bool mAppPaused = false;
	bool mMinimized = false;
	bool mMaximized = false;
	bool mFullscreenState = false;

	HINSTANCE mhAppInst = nullptr; 
	HWND mhMainWnd = nullptr; 

	// ������ �Ӽ�
	std::wstring mApplicationName = L"D3DFramework";
	int mScreenWidth = 800;
	int mScreenHeight = 600;

	GameTimer mTimer;
	class InputManager* inputManager;
};

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);