#pragma once

#include "pch.h"
#include "Object.h"
#include "Interface.h"

class Terrain : public Object, public Renderable
{
public:
	Terrain(std::string&& name);
	virtual ~Terrain();

public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;

public:
	void BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float width, float depth, int xLen, int zLen);

public:
	inline void SetMaterial(class Material* material) { mMaterial = material; }
	inline class Material* GetMaterial() { return mMaterial; }

protected:
	std::unique_ptr<class Mesh> mTerrainMesh;
	class Material* mMaterial = nullptr;
};