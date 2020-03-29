#include <Component/Widget.h>
#include <Component/Mesh.h>
#include <Framework/D3DInfo.h>
#include <vector>

Widget::Widget(std::string&& name, ID3D12Device* device, ID3D12GraphicsCommandList* commandList) : Component(std::move(name))
{
	mesh = std::make_unique<Mesh>(GetName() + R"("Mesh")" + std::to_string(GetUID()));

	std::vector<UINT16> indices;
	indices.emplace_back(0);
	indices.emplace_back(1);
	indices.emplace_back(2);

	indices.emplace_back(2);
	indices.emplace_back(3);
	indices.emplace_back(1);

	mesh->BuildIndices(device, commandList, indices.data(), (UINT16)indices.size(), (UINT16)sizeof(UINT16));
}

Widget::~Widget() { }

void Widget::Render(ID3D12GraphicsCommandList* commandList, BoundingFrustum* frustum) const
{
	if (isVisible)
	{
		mesh->Render(commandList);
	}
}

void Widget::SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = startAddress + cbIndex * ConstantsSize::objectCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView((int)RpCommon::Object, cbAddress);
}

void Widget::SetPosition(const INT32 posX, const INT32 posY)
{
	this->posX = posX;
	this->posY = posY;
}

void Widget::SetSize(const UINT32 width, const UINT32 height)
{
	this->width = width;
	this->height = height;
}

void Widget::SetAnchor(const float anchorX, const float anchorY)
{
	this->anchorX = anchorX;
	this->anchorY = anchorY;
}

Mesh* Widget::GetMesh() const
{
	return mesh.get();
}

void Widget::SetMaterial(Material* material)
{
	this->material = material;
}

Material* Widget::GetMaterial() const
{
	return material;
}