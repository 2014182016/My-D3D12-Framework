#pragma once

#define DIRECTINPUT_VERSION 0x0800

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dinput8.lib")

#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <mmsystem.h> // dsound.h를 인클루드하기 전 필수
#include <dsound.h>
#include <dinput.h>

#include <string>
#include <wrl.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <memory>
#include <basetsd.h>
#include <thread>

#include "../Framework/d3dx12.h"
#include "../Framework/Vector.h"
#include "../Framework/D3DUtil.h"


namespace DirectX
{
	std::ostream& operator<<(std::ostream& os, const XMFLOAT3& xmf);
	std::ostream& operator<<(std::ostream& os, const XMFLOAT4X4& xmf4x4f);
}