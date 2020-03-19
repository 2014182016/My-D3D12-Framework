#pragma once

#include "pch.h"

class AssetLoader
{
public:
	static bool LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::wstring fileName,
		std::vector<struct Vertex>& vertices, std::vector<std::uint16_t>& indices);
	static bool ConvertObj(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring fileName);
	static void ObjToH3d(const std::vector<struct Vertex>& vertices, const std::vector<std::uint16_t> indices, std::wstring fileName);
	static void CalculateTBN(const struct VertexBasic& v1, const struct VertexBasic& v2, const struct VertexBasic& v3,
		DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);

	static FILE* LoadWave(IDirectSound8* d3dSound, const std::string fileName, struct WaveHeaderType& header);
};