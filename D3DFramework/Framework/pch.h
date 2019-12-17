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

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "pdh.lib")

#include <windows.h>
#include <wrl.h>
#include <Pdh.h>
#include <dxgi1_4.h>
#include <d3d12.h>
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

	static std::ostream& operator<<(std::ostream& os, const DirectX::XMFLOAT3& xmf)
	{
		os << xmf.x << " " << xmf.y << " " << xmf.z;
		return os;
	}

	static std::ostream& operator<<(std::ostream& os, const DirectX::XMFLOAT4X4& xmf4x4f)
	{
		os << xmf4x4f._11 << " " << xmf4x4f._12 << " " << xmf4x4f._13 << " " << xmf4x4f._14 << std::endl;
		os << xmf4x4f._21 << " " << xmf4x4f._22 << " " << xmf4x4f._23 << " " << xmf4x4f._24 << std::endl;
		os << xmf4x4f._31 << " " << xmf4x4f._32 << " " << xmf4x4f._33 << " " << xmf4x4f._34 << std::endl;
		os << xmf4x4f._41 << " " << xmf4x4f._42 << " " << xmf4x4f._43 << " " << xmf4x4f._44;
		return os;
	}
}


#endif //PCH_H
