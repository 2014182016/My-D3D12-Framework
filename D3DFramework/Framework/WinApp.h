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

	// ���� ���α׷� ����
	bool mAppPaused = false; 
	bool mMinimized = false; 
	bool mMaximized = false; 
	bool mFullscreenState = false;

	// Ű�� ���� ���¸� �����Ѵ�.
	bool mKeys[256];

	// ������ �Ӽ�
	std::wstring mApplicationName = L"D3DFramework";
	int mScreenWidth = 800;
	int mScreenHeight = 600;

	// �ð��� �����ϴ� Ÿ�̸�
	GameTimer mTimer;
};

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);