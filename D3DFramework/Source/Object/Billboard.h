#pragma once

#include "GameObject.h"

class Mesh;

/*
객체가 렌더링될 때, 2D 평면으로 항상 카메라를
향하도록 하는 물체
*/
class Billboard : public GameObject
{
public:
	Billboard(std::string&& name);
	virtual ~Billboard();

public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const override;

public:
	// 빌보드 전용 메쉬를 생성한다.
	void BuildBillboardMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

public:
	XMFLOAT2 mSize = { 1.0f, 1.0f };

private:
	std::unique_ptr<Mesh> billboardMesh = nullptr;
};