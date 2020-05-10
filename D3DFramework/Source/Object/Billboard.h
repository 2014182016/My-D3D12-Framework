#pragma once

#include "GameObject.h"

class Mesh;

/*
��ü�� �������� ��, 2D ������� �׻� ī�޶�
���ϵ��� �ϴ� ��ü
*/
class Billboard : public GameObject
{
public:
	Billboard(std::string&& name);
	virtual ~Billboard();

public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const override;

public:
	// ������ ���� �޽��� �����Ѵ�.
	void BuildBillboardMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

public:
	XMFLOAT2 mSize = { 1.0f, 1.0f };

private:
	std::unique_ptr<Mesh> billboardMesh = nullptr;
};