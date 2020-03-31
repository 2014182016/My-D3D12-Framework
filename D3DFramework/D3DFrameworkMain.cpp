#include "Source/Framework/D3DFramework.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	// �ǽð� �޸� �˻縦 Ȱ��ȭ�Ѵ�.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		D3DFramework framework(hInstance, 1280, 900, L"D3DFramework");
		if (!framework.Initialize())
			return 0;

		return framework.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}
