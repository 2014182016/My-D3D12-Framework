#include "pch.h"
#include "Billboard.h"
#include "Mesh.h"
#include "Structures.h"

using namespace DirectX;
using namespace std::literals;

Billboard::Billboard(std::string&& name) : GameObject(std::move(name)) 
{
	mCollisionType = CollisionType::Point;
}

Billboard::~Billboard() { }

void Billboard::BuildBillboardMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mBillboardMesh.reset();
	mBillboardMesh = std::make_unique<Mesh>(GetName() + "Mesh"s + std::to_string(GetUID()));

	std::vector<BillboardVertex> vertices;
	vertices.emplace_back(GetPosition(), mSize);

	mBillboardMesh->BuildVertices(device, commandList, (void*)vertices.data(), 1, (UINT)sizeof(BillboardVertex));
	mBillboardMesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	mMesh = mBillboardMesh.get();
}

void Billboard::Render(ID3D12GraphicsCommandList* commandList)
{
	mMesh->Render(commandList, 1, false);
}