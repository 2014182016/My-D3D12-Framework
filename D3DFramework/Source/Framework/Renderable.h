#pragma once

#include <Framework/D3DUtil.h>

/*
�׸� �� �ִ� ������Ʈ�� �� �������̽��� ��ӹ޴´�.
*/
class Renderable
{
public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr) const = 0;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const = 0;
};