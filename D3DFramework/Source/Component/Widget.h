#pragma once

#include "Component.h"

class Widget : public Component
{
public:
	Widget(std::string&& name);
	virtual ~Widget();

public:
	void SetPosition(std::uint32_t x, std::uint32_t y);
	void SetSize(std::uint32_t width, std::uint32_t height);
	void SetAnchor(float x, float y);
	void SetMaterial(class Material* material);

	inline std::uint32_t GetPosX() const { return mPosX; }
	inline std::uint32_t GetPosY() const { return mPosY; }
	inline std::uint32_t GetWidth() const { return mWidth; }
	inline std::uint32_t GetHeight() const { return mHeight; }
	inline float GetAnchorX() const { return mAnchorX; }
	inline float GetAnchotY() const { return mAnchorY; }

	inline class Material* GetMaterial() const { return mMaterial; }
	inline static class MeshGeometry* GetMesh() { return mMesh; }

	inline void SetIsVisible(bool value) { mIsVisible = value; }
	inline bool GetIsVisible() const { return mIsVisible; }

protected:
	std::uint32_t mPosX = 0;
	std::uint32_t mPosY = 0;
	std::uint32_t mWidth = 0;
	std::uint32_t mHeight = 0;
	float mAnchorX = 0.0f;
	float mAnchorY = 0.0f;

private:
	static inline class MeshGeometry* mMesh = nullptr;
	class Material* mMaterial = nullptr;

	bool mIsVisible = true;
};