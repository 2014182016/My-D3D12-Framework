#pragma once

#ifndef PCH_H
#define PCH_H

// 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define DIRECTINPUT_VERSION 0x0800

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dinput8.lib")

#include <windows.h>
#include <wrl.h>
#include <Pdh.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <mmsystem.h>
#include <dsound.h>
#include <dinput.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <functional>
#include <thread>
#include <array>
#include <vector>
#include <list>
#include <forward_list>
#include <unordered_map>
#include <bitset>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <any>
#include <optional>
#include <float.h>
#include <random>
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "GeometryGenerator.h"

#define SWAP_CHAIN_BUFFER_COUNT 2
#define NUM_FRAME_RESOURCES 3

namespace DirectX
{
	static DirectX::XMFLOAT4X4 Identity4x4f()
	{
		static DirectX::XMFLOAT4X4 I(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		return I;
	}

	std::ostream& operator<<(std::ostream& os, const DirectX::XMFLOAT3& xmf);
	std::ostream& operator<<(std::ostream& os, const DirectX::XMFLOAT4X4& xmf4x4f);

	DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& lhs, const float rhs);
	DirectX::XMFLOAT3 operator*(const DirectX::XMFLOAT3& lhs, const float rhs);
	DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
	DirectX::XMFLOAT3 operator-(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
	DirectX::XMFLOAT3 operator/(const DirectX::XMFLOAT3& lhs, const float rhs);
}


#endif //PCH_H
