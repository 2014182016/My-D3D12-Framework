#include "pch.h"
#include "Widget.h"
#include "MeshGeometry.h"
#include "Structures.h"

using namespace std::literals;
using namespace DirectX;

Widget::Widget(std::string&& name) : Component(std::move(name)) { }

Widget::~Widget() { }

void Widget::BuildWidgetMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mMesh = std::make_unique<MeshGeometry>(GetName() + "Mesh"s + std::to_string(GetUID()));

	std::vector<std::uint16_t> indices;
	indices.emplace_back(0);
	indices.emplace_back(1);
	indices.emplace_back(2);

	indices.emplace_back(2);
	indices.emplace_back(3);
	indices.emplace_back(1);

	mMesh->BuildIndices(device, commandList, indices.data(), (UINT)indices.size(), (UINT)sizeof(std::uint16_t));
}

void Widget::SetPosition(std::uint32_t x, std::uint32_t y)
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