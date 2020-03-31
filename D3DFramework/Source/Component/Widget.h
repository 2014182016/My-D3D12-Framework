#pragma once

#include "Component.h"
#include "../Framework/Renderable.h"

class Material;
class Mesh;

/*
화면 상에 가장 앞쪽에 그려지는 사각형 메쉬
렌더링에서 가장 마지막에 그려지므로 항상 보인다.
*/
class Widget : public Component, public Renderable
{
public:
	Widget(std::string&& name, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual ~Widget();

public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;

	void SetPosition(const INT32 posX, const INT32 posY);
	void SetSize(const UINT32 width, const UINT32 height);
	void SetAnchor(const float anchorX, const float anchorY);

	Mesh* GetMesh() const;
	void SetMaterial(Material* material);
	Material* GetMaterial() const;

public:
	INT32 posX = 0;
	INT32 posY = 0;
	UINT32 width = 0;
	UINT32 height = 0;
	float anchorX = 0.0f;
	float anchorY = 0.0f;

	UINT32 cbIndex = 0;
	bool isVisible = true;

private:
	std::unique_ptr<Mesh> mesh = nullptr;
	Material* material = nullptr;
};