#include <Object/Billboard.h>
#include <Component/Mesh.h>
#include <vector>

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

void Billboard::Render(ID3D12GraphicsCommandList* commandList)
{
	if (isVisible)
	{
		GetMesh()->Render(commandList, 1, false);
	}
}