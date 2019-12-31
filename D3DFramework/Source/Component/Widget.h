#pragma once

#include "Component.h"

class Widget : public Component
{
public:
	Widget(std::string&& name);
	virtual ~Widget();

public:
	// 동적 정점 버퍼를 사용하기 위해 만드는 임시 메쉬
	void BuildWidgetMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	void SetPosition(std::uint32_t x, std::uint32_t y);
	void SetSize(std::uint32_t width, std::uint32_t height);
	void SetAnchor(float x, float y);
public:
	inline void SetWidgetIndex(UINT index) { mWidgetIndex = index; }
	inline UINT GetWidgetIndex() const { return mWidgetIndex; }

	inline std::uint32_t GetPosX() const { return mPosX; }
	inline std::uint32_t GetPosY() const { return mPosY; }
	inline std::uint32_t GetWidth() const { return mWidth; }
	inline std::uint32_t GetHeight() const { return mHeight; }
	inline float GetAnchorX() const { return mAnchorX; }
	inline float GetAnchorY() const { return mAnchorY; }

	inline void SetMaterial(class Material* material) { mMaterial = material; }
	inline class Material* GetMaterial() const { return mMaterial; }
	inline class MeshGeometry* GetMesh() { return mMesh.get(); }

	inline void SetIsVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }

protected:
	std::uint32_t mPosX = 0;
	std::uint32_t mPosY = 0;
	std::uint32_t mWidth = 0;
	std::uint32_t mHeight = 0;
	float mAnchorX = 0.0f;
	float mAnchorY = 0.0f;

	std::unique_ptr<class MeshGeometry> mMesh = nullptr;
	class Material* mMaterial = nullptr;

private:
	UINT mWidgetIndex = 0;

	bool mIsVisible = true;
};