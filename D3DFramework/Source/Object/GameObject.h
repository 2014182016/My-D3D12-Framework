#pragma once

#include "Object.h"
#include "Interface.h"

class GameObject : public Object, public Renderable
{
public:
	GameObject(std::string&& name);
	virtual ~GameObject();

public:
	virtual void BeginPlay() override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;
	virtual void WorldUpdate() override;
	virtual void Collide(class GameObject* other);

public:
	bool IsInFrustum(DirectX::BoundingFrustum* frustum) const;
	bool IsCollision(class GameObject* other) const;

public:
	inline class Mesh* GetMesh() { return mMesh; }
	inline class Material* GetMaterial() { return mMaterial; }
	inline void SetMesh(class Mesh* mesh) { mMesh = mesh; }
	inline void SetMaterial(class Material* material) { mMaterial = material; }

	inline void SetVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }

	inline void SetCBIndex(UINT index) { mCBIndex = index; }
	inline UINT GetCBIndex() const { return mCBIndex; }

	inline RenderLayer GetRenderLayer() const { return mRenderLayer; }
	inline void SetRenderLayer(RenderLayer layer) { mRenderLayer = layer; }

	inline CollisionType GetCollisionType() const { return mCollisionType; }
	void SetCollisionEnabled(bool value);
	CollisionType GetMeshCollisionType() const;

	inline const std::any& GetCollisionBounding() const { return mCollisionBounding; }
	std::optional<DirectX::XMMATRIX> GetBoundingWorld() const;

protected:
	std::any mCollisionBounding = nullptr;
	CollisionType mCollisionType = CollisionType::None;

	class Mesh* mMesh = nullptr;
	class Material* mMaterial = nullptr;

private:
	RenderLayer mRenderLayer = RenderLayer::Opaque;
	UINT mCBIndex = 0;
	bool mIsVisible = true;
};