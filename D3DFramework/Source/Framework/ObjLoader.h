#pragma once

#include "pch.h"
#include "Structure.h"

class ObjLoader
{
public:
	static std::unique_ptr<class Mesh> LoadH3d(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshInfo& modelInfo);
	static void ConvertObj(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const MeshInfo& modelInfo);
	static void ObjToH3d(const std::vector<Vertex>& vertices, const std::vector<std::uint16_t> indices, std::wstring fileName);
	static void CalculateTBN(const VertexBasic& v1, const VertexBasic& v2, const VertexBasic& v3,
		DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);
};