#pragma once

#include "Base.h"

class Material : public Base
{
public:
	Material(std::string&& name);
	virtual ~Material();
	
public:
	void SetUV(float u, float v);
	void Move(float u, float v);
	void Rotate(float rotationU, float rotationV);
	void Scale(float scaleU, float scaleV);

public:
	DirectX::XMMATRIX GetMaterialTransform() const;
	inline DirectX::XMFLOAT4X4 GetMaterialTransform4x4f() const { return mMatTransform; }

	inline DirectX::XMFLOAT4 GetDiffuse() const { return mDiffuseAlbedo; }
	inline void SetDiffuse(DirectX::XMFLOAT3 diffuse) { mDiffuseAlbedo = DirectX::XMFLOAT4(diffuse.x, diffuse.y, diffuse.z, 1.0f); }
	void SetDiffuse(float r, float g, float b);

	inline DirectX::XMFLOAT3 GetSpecular() const { return mSpecular; }
	inline void SetSpecular(DirectX::XMFLOAT3 specular) { mSpecular = specular; }
	void SetSpecular(float r, float g, float b);

	inline float GetRoughness() const { return mRoughness; }
	inline void SetRoughtness(float roughness) { mRoughness = roughness; }

	inline float GetOpacity() const { return mDiffuseAlbedo.w; }
	inline void SetOpacity(float opacity) { mDiffuseAlbedo.w = opacity; }

	inline int GetDiffuseIndex() const { return mDiffuseSrvHeapIndex; }
	inline void SetDiffuseIndex(int index) { mDiffuseSrvHeapIndex = index; }
	inline int GetNormalIndex() const { return mNormalSrvHeapIndex; }
	inline void SetNormalIndex(int index) { mNormalSrvHeapIndex = index; }

	inline int GetMaterialIndex() const { return mMatCBIndex; }

protected:
	DirectX::XMFLOAT4X4 mMatTransform = DirectX::Identity4x4f();

	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 mSpecular = { 0.01f, 0.01f, 0.01f };
	float mRoughness = 0.25f;

private:
	// 디퓨즈 텍스처를 위한 SRV 힙에서의 인덱스
	int mDiffuseSrvHeapIndex = -1;
	// 노말 텍스처를 위한 SRV 힙에서의 인덱스
	int mNormalSrvHeapIndex = -1;

	// 머터리얼을 생성할 때마다 삽입할 상수 버퍼에서의 인덱스
	static inline int mCurrentIndex = 0;
	// 이 머터리얼에 일치하는 상수 버퍼에서의 인덱스
	int mMatCBIndex = -1;
};
