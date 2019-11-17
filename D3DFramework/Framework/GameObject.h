#pragma once

#include "Object.h"

class GameObject : public Object
{
public:
	GameObject(std::string name);
	virtual ~GameObject();
	
public:
	virtual void Destroy() override;
	virtual void WorldUpdate() override;

public:
	inline class Material* GetMaterial() const { return mMaterial; }
	inline void SetMaterial(class Material* mat) { mMaterial = mat; }
	inline class MeshGeometry* GetMesh() const { return mMesh; }
	inline void SetMesh(class MeshGeometry* mesh) { mMesh = mesh; }

	inline RenderLayer GetRenderLayer() const { return mRenderLayer; }
	inline void SetRenderLayer(RenderLayer layer) { mRenderLayer = layer; }

	inline void SetVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }

	inline const std::any& GetCollisionBounding() const { return mWorldBounding; }
	std::optional<DirectX::XMMATRIX> GetBoundingWorld() const;

protected:
	class Material* mMaterial = nullptr;
	class MeshGeometry* mMesh = nullptr;

	std::any mWorldBounding = nullptr;

private:
	RenderLayer mRenderLayer = RenderLayer::Opaque;

	bool mIsVisible = true;
};