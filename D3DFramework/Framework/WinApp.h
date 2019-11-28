#pragma once

#include "pch.h"
#include "Enums.h"

class WinApp
{
public:
	WinApp(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName);
	WinApp(const WinApp& rhs) = delete;
	WinApp& operator=(const WinApp& rhs) = delete;
	virtual ~WinApp();

public:
	virtual bool Initialize();
	virtual void OnDestroy() { };

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	int Run();

protected:
	virtual void OnResize(int screenWidth, int screenHeight);
	virtual void Tick(float deltaTime) { };
	virtual void Render() { };
	virtual void ApplyOption(Option option) { };

public:
	inline static WinApp* GetApp() { return mApp; }

	inline float GetAspectRatio() const { return static_cast<float>(mScreenWidth) / mScreenHeight; }
	inline HINSTANCE GetAppInst() const { return mhAppInst; }
	inline HWND GetMainWnd() const { return mhMainWnd; }

	inline bool GetOptionEnabled(Option option) const { return mOptions.test((int)option); }
	void SwitchOptionEnabled(Option option);
	void SetOptionEnabled(Option option, bool value);
	

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

	HINSTANCE mhAppInst = nullptr; 
	HWND mhMainWnd = nullptr; 

	// ������ �Ӽ�
	std::wstring mApplicationName = L"D3DFramework";
	int mScreenWidth = 800;
	int mScreenHeight = 600;

	// option���� bool���� �����ϴ� �����̳�
	std::bitset<(int)Option::Count> mOptions;

	std::unique_ptr<class GameTimer> mGameTimer;
	std::unique_ptr<class InputManager> mInputManager;
};

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);