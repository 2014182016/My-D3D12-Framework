#pragma once

#include "Component.h"

class Material : public Component
{
public:
	Material(std::string&& name);
	virtual ~Material();
	
public:
	void SetUV(float u, float v);
	void Move(float u, float v);
	void Rotate(float degree);
	void SetScale(float scaleU, float scaleV);

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
	inline void SetRoughness(float roughness) { mRoughness = roughness; }

	inline float GetOpacity() const { return mDiffuseAlbedo.w; }
	inline void SetOpacity(float opacity) { mDiffuseAlbedo.w = opacity; }

	inline int GetDiffuseMapIndex() const { return mDiffuseMapIndex; }
	inline void SetDiffuseMapIndex(int index) { mDiffuseMapIndex = index; }
	inline int GetNormalMapIndex() const { return mNormalMapIndex; }
	inline void SetNormalMapIndex(int index) { mNormalMapIndex = index; }
	inline int GetHeightMapIndex() const { return mHeightMapIndex; }
	inline void SetHeightMapIndex(int index) { mHeightMapIndex = index; }

	inline UINT GetMaterialIndex() const { return mMaterialIndex; }

protected:
	DirectX::XMFLOAT4X4 mMatTransform = DirectX::Identity4x4f();

	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 mSpecular = { 0.01f, 0.01f, 0.01f };
	float mRoughness = 0.25f;

private:
	// ��ǻ��� �ؽ�ó�� ���� SRV �������� �ε���
	int mDiffuseMapIndex = -1;
	// �븻�� �ؽ�ó�� ���� SRV �������� �ε���
	int mNormalMapIndex = -1;
	// ����Ʈ�� �ؽ�ó�� ���� SRV �������� �ε���
	int mHeightMapIndex = -1;

	// ���͸����� ������ ������ ������ ��� ���ۿ����� �ε���
	static inline UINT currentMaterialIndex = 0;
	// �� ���͸��� ��ġ�ϴ� ��� ���ۿ����� �ε���
	UINT mMaterialIndex = -1;
};
