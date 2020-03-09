#include "pch.h"
#include "Widget.h"
#include "Mesh.h"
#include "Structure.h"
#include "Global.h"

using namespace std::literals;
using namespace DirectX;

Widget::Widget(std::string&& name, ID3D12Device* device, ID3D12GraphicsCommandList* commandList) : Component(std::move(name))
{
	mMesh = std::make_unique<Mesh>(GetName() + "Mesh"s + std::to_string(GetUID()));

	std::vector<std::uint16_t> indices;
	indices.emplace_back(0);
	indices.emplace_back(1);
	indices.emplace_back(2);

	indices.emplace_back(2);
	indices.emplace_back(3);
	indices.emplace_back(1);

	mMesh->BuildIndices(device, commandList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));
}

Widget::~Widget() { }

void Widget::SetPosition(int x, int y)
{
	mPosX = x;
	mPosY = y;
}

void Widget::SetSize(std::uint32_t width, std::uint32_t height)
{
	mWidth = width;
	mHeight = height;
}

void Widget::SetAnchor(float x, float y)
{
	mAnchorX = x;
	mAnchorY = y;
}

void Widget::Render(ID3D12GraphicsCommandList* commandList, DirectX::BoundingFrustum* frustum) const
{
	if (mIsVisible)
	{
		mMesh->Render(commandList);
	}
}

void Widget::SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = startAddress + mCBIndex * ConstantsSize::objectCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView((UINT)RpCommon::Object, cbAddress);
}