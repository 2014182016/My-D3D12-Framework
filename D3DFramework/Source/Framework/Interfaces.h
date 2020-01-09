#pragma once

#include "pch.h"
#include "Enums.h"

#define SHADOW_MAP_SIZE 1024

class Renderable
{
public:
	virtual void Render(ID3D12GraphicsCommandList* commandList) = 0;
	virtual bool IsInFrustum(DirectX::BoundingFrustum* camFrustum) { return true; }

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


protected:
	class Mesh* mMesh = nullptr;
	class Material* mMaterial = nullptr;

	RenderLayer mRenderLayer = RenderLayer::Opaque;
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

class ShadowMap
{
public:
	virtual void BuildDescriptors(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv) = 0;
	virtual void BuildDescriptors(ID3D12Device* device) = 0;
	virtual void BuildResource(ID3D12Device* device) = 0;
	virtual void OnResize(ID3D12Device* device, UINT width, UINT height) = 0;
	virtual void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList) = 0;

public:
	inline void SetFrustum(const DirectX::BoundingFrustum& frustum) { mLightFrustum = frustum; }

protected:
	DirectX::BoundingFrustum mLightFrustum;
};