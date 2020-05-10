#include "../PrecompiledHeader/pch.h"
#include "Billboard.h"
#include "../Component/Mesh.h"


Billboard::Billboard(std::string&& name) : GameObject(std::move(name)) 
{
	collisionType = CollisionType::Point;
}

Billboard::~Billboard() { }

void Billboard::BuildBillboardMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	billboardMesh.reset();
	billboardMesh = std::make_unique<Mesh>(GetName() + R"("Mesh")" + std::to_string(GetUID()));

	std::vector<BillboardVertex> vertices;
	vertices.emplace_back(GetPosition(), mSize);

	billboardMesh->BuildVertices(device, commandList, (void*)vertices.data(), 1, (UINT)sizeof(BillboardVertex));
	billboardMesh->SetPrimitiveType(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	SetMesh(billboardMesh.get());
}

void Billboard::Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum) const
{
	if (isVisible)
	{
		GetMesh()->Render(cmdList, 1, false);
	}
}