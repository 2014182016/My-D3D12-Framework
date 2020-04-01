#pragma once

#include "Component.h"
#include "../Framework/Vector.h"

/*
��ü�� ������ ǥ���ϴ� Ŭ����
*/
class Material : public Component
{
public:
	Material(std::string&& name);
	virtual ~Material();
	
public:
	// UV�� �ش� ������ �����Ѵ�.
	void SetUV(float u, float v);
	// UV�� �ش� ����ŭ �̵���Ų��.
	void Move(float u, float v);
	// ���͸����� �ݽð�������� degree��ŭ ȸ����Ų��.
	void Rotate(float degree);
	// ���͸����� ũ�⸦ ���̰ų� Ű���.
	void SetScale(float scaleU, float scaleV);

	XMMATRIX GetMaterialTransform() const;
	XMFLOAT4X4 GetMaterialTransform4x4f() const;
	UINT32 GetMaterialIndex() const;

public:
	// ��ǻ��� �ؽ�ó�� ���� SRV �������� �ε���
	INT32 diffuseMapIndex = -1;
	// �븻�� �ؽ�ó�� ���� SRV �������� �ε���
	INT32 normalMapIndex = -1;
	// ����Ʈ�� �ؽ�ó�� ���� SRV �������� �ε���
	INT32 heightMapIndex = -1;

	// ���̵��� ���Ǵ� ���� ��� ���� �ڷ�
	XMFLOAT4 diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 specular = { 0.01f, 0.01f, 0.01f };
	float roughness = 0.25f;

protected:
	XMFLOAT4X4 matTransform = Matrix4x4::Identity();

private:
	// ���͸����� ������ ������ ������ ��� ���ۿ����� �ε���
	static inline UINT32 currentMaterialIndex = 0;

private:
	// �� ���͸��� ��ġ�ϴ� ��� ���ۿ����� �ε���
	UINT32 materialIndex = -1;
};
