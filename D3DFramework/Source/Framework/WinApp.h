#pragma once

#pragma comment(lib, "pdh.lib")

#include <string>
#include <Pdh.h>

// ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

class GameTimer;
class InputManager;

/*
WinApi�� ����ϱ� ���� Ŭ����
������ ���¸� �����ϰ�, ���� ������ �����Ѵ�.
*/
class WinApp
{
public:
	WinApp(HINSTANCE hInstance, const INT32 screenWidth, const INT32 screenHeight, 
		const std::wstring applicationName, const bool useWinApi = true);
	WinApp(const WinApp& rhs) = delete;
	WinApp& operator=(const WinApp& rhs) = delete;
	virtual ~WinApp();

public:
	virtual bool Initialize();
	virtual void OnDestroy() { };

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	static WinApp* GetInstance();
	int Run();

protected:
	virtual void OnResize(const INT32 screenWidth, const INT32 screenHeight);
	virtual void Tick(float deltaTime);
	virtual void Render() { };

public:
	void Update();
	
	float GetAspectRatio() const;
	HINSTANCE GetAppInst() const;
	INT32 GetScreenWidth() const;
	INT32 GetScreenHeight() const;

	void SetMainWnd(HWND hwnd);
	HWND GetMainWnd() const;

	GameTimer* GetGameTimer();

private:
	bool InitMainWindow();
	void CalculateFrameStats();

protected:
	// ���� ���α׷� ����
	bool appPaused = false;
	bool minimized = false;
	bool maximized = false;
	bool useWinApi = true;

	HINSTANCE hAppInst = nullptr; 
	HWND hMainWnd = nullptr; 

	// cpu ��뷮�� ����ϱ� ���� PDH(Performance Data Helper)�� �̿��Ѵ�.
	HQUERY queryHandle;
	HCOUNTER counterHandle;

	// ������ �Ӽ�
	std::wstring applicationName = L"D3DFramework";
	INT32 screenWidth = 800;
	INT32 screenHeight = 600;

	std::unique_ptr<GameTimer> gameTimer = nullptr;

private:
	static inline WinApp* instance = nullptr;
};

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);