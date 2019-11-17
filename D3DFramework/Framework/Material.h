#pragma once

#include "pch.h"
#include "Base.h"

class Material : public Base
{
public:
	Material(std::string name);
	virtual ~Material();

public:
	DirectX::XMMATRIX GetMaterialTransform() const;
	inline DirectX::XMFLOAT4X4 GetMaterialTransform4x4f() const { return mMatTransform; }

	inline DirectX::XMFLOAT4 GetDiffuse() const { return mDiffuseAlbedo; }
	inline void SetDiffuse(DirectX::XMFLOAT4 diffuse) { mDiffuseAlbedo = diffuse; }
	void SetDiffuse(float r, float g, float b, float a);

	inline DirectX::XMFLOAT3 GetSpecular() const { return mSpecular; }
	inline void SetSpecular(DirectX::XMFLOAT3 specular) { mSpecular = specular; }
	void SetSpecular(float r, float g, float b);

	inline float GetRoughness() const { return mRoughness; }
	inline void SetRoughtness(float roughness) { mRoughness = roughness; }

	inline int GetDiffuseIndex() const { return mDiffuseSrvHeapIndex; }
	inline void SetDiffuseIndex(int index) { mDiffuseSrvHeapIndex = index; }
	inline int GetNormalIndex() const { return mNormalSrvHeapIndex; }
	inline void SetNormalIndex(int index) { mNormalSrvHeapIndex = index; }

	inline int GetMaterialIndex() const { return mMatCBIndex; }

protected:
	DirectX::XMFLOAT4X4 mMatTransform = DirectX::Identity4x4f();

	DirectX::XMFLOAT4 mDiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 mSpecular = { 0.01f, 0.01f, 0.01f };
	float mRoughness = .25f;

private:
	// ��ǻ�� �ؽ�ó�� ���� SRV �������� �ε���
	int mDiffuseSrvHeapIndex = -1;
	// �븻 �ؽ�ó�� ���� SRV �������� �ε���
	int mNormalSrvHeapIndex = -1;

	// ���͸����� ������ ������ ������ ��� ���ۿ����� �ε���
	static int mCurrentIndex;
	// �� ���͸��� ��ġ�ϴ� ��� ���ۿ����� �ε���
	int mMatCBIndex = -1;
};
