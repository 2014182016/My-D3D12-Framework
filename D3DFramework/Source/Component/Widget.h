#pragma once

#include "Component.h"
#include "Interface.h"

class Widget : public Component, public Renderable
{
public:
	Widget(std::string&& name, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual ~Widget();

public:
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr) const override;
	virtual void SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const override;

public:
	void SetPosition(int x, int y);
	void SetSize(std::uint32_t width, std::uint32_t height);
	void SetAnchor(float x, float y);

public:
	inline int GetPosX() const { return mPosX; }
	inline int GetPosY() const { return mPosY; }
	inline std::uint32_t GetWidth() const { return mWidth; }
	inline std::uint32_t GetHeight() const { return mHeight; }
	inline float GetAnchorX() const { return mAnchorX; }
	inline float GetAnchorY() const { return mAnchorY; }

	inline class Mesh* GetMesh() { return mMesh.get(); }
	inline class Material* GetMaterial() { return mMaterial; }
	inline void SetMaterial(class Material* material) { mMaterial = material; }

	inline void SetVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }

	inline void SetCBIndex(UINT index) { mCBIndex = index; }
	inline UINT GetCBIndex() const { return mCBIndex; }

protected:
	int mPosX = 0;
	int mPosY = 0;
	std::uint32_t mWidth = 0;
	std::uint32_t mHeight = 0;
	float mAnchorX = 0.0f;
	float mAnchorY = 0.0f;

private:
	std::unique_ptr<class Mesh> mMesh;
	class Material* mMaterial = nullptr;

	UINT mCBIndex = 0;
	bool mIsVisible = true;
};