#pragma once

#include "pch.h"

class Renderable
{
public:
	virtual void Render(ID3D12GraphicsCommandList* commandList) = 0;

	inline class Mesh* GetMesh() { return mMesh; }
	inline class Material* GetMaterial() { return mMaterial; }
	inline void SetMesh(class Mesh* mesh) { mMesh = mesh; }
	inline void SetMaterial(class Material* material) { mMaterial = material; }

	inline void SetVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }

	inline void SetCBIndex(UINT index) { mCBIndex = index; }
	inline UINT GetCBIndex() const { return mCBIndex; }

protected:
	class Mesh* mMesh = nullptr;
	class Material* mMaterial = nullptr;
	UINT mCBIndex = 0;
	bool mIsVisible = true;
};

class Physics
{
public:
	inline void SetPhysics(bool value) { mIsPhysics = value; }
	inline bool GetIsPhysics() const { return mIsPhysics; }

	inline void SetMass(float mass) { mMass = mass; mInvMass = 1.0f / mass; }
	inline float GetMass() const { return mMass; }
	inline float GetInvMass() const { return mInvMass; }

	inline void SetCof(float cof) { mCof = cof; }
	inline float GetCof() const { return mCof; }

	inline void SetVelocity(DirectX::XMFLOAT3 velocity) { mVelocity = velocity; }
	inline DirectX::XMFLOAT3 GetVelocity() const { return mVelocity; }

protected:
	DirectX::XMFLOAT3 mVelocity = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 mAcceleration = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	float mMass = 0.0f;
	float mInvMass = 0.0f;
	float mCof = 0.6f; 	// Coefficient of Restitution(반발 계수)
	bool mIsPhysics = false;
};