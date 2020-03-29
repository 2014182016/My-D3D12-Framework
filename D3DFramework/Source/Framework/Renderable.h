#pragma once

#include <Framework/D3DUtil.h>

/*
그릴 수 있는 오브젝트는 이 인터페이스를 상속받는다.
*/
class Renderable
{
public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const = 0;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const = 0;
};