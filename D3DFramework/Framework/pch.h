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

#include <windows.h>
#include <wrl.h>
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
#include <array>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <map>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <any>
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "GeometryGenerator.h"

#define SWAP_CHAIN_BUFFER_COUNT 2
#define NUM_FRAME_RESOURCES 3

#endif //PCH_H
