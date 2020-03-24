#pragma once

#include "GameObject.h"

class Billboard : public GameObject
{
public:
	Billboard(std::string&& name);
	virtual ~Billboard();

public:
	virtual void Render(ID3D12GraphicsCommandList* commandList);

public:
	void BuildBillboardMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

public:
	inline void SetSize(const DirectX::XMFLOAT2& size) { mSize = size; }
	inline DirectX::XMFLOAT2 GetSize() const { return mSize; }

protected:
	DirectX::XMFLOAT2 mSize;

private:
	std::unique_ptr<class Mesh> mBillboardMesh;

};