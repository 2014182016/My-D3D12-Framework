#pragma once

#include "Object.h"

class ShadowMap;

#define SHADOW_MAP_SIZE 1024

/*
���� �Ӽ��� �����ϴ� �⺻ Ŭ����
3���� ������ ���� �� Ŭ������ ��� �޴´�.
*/
class Light : public Object
{
public:
	Light(std::string&& name);
	virtual ~Light();

public:
	// Light�� Ÿ�Կ� ���� LightConstants�� ä���� �� ������ �ٸ���.
	// �� �Լ��� override�Ͽ� Ÿ�Կ� �´� �����͸� ä���־��ش�.
	virtual void SetLightData(LightData& lightData);

public:
	// ������ ���� �׸���.
	void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList);
	ShadowMap* GetShadowMap();

	XMMATRIX GetView() const;
	XMMATRIX GetProj() const;

public:
	// ���̴����� ����ϱ� ���� ����Ʈ �Ӽ���
	XMFLOAT3 strength = { 0.5f, 0.5f, 0.5f };
	float falloffStart = 1.0f;
	float falloffEnd = 10.0f;
	float spotAngle = 1.0f;
	bool enabled = true;

	// ������ �ʿ��� ���� ����� ũ��
	XMFLOAT2 shadowMapSize = { 50.0f, 50.0f };

protected:
	// NDC ���� [-1, 1]^2�� �ؽ�ó ���� [0, 1]^2���� ��ȯ�ϴ� ���
	static inline const XMMATRIX toTextureTransform =
		XMMATRIX(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	// ����Ʈ ������ ������ ��
	std::unique_ptr<ShadowMap> shadowMap = nullptr;
	// ������ ���� �׸��� ���� ��ü�� �������� �ø��ϱ� ���� ���
	BoundingFrustum lightFrustum;

	// ����Ʈ ���������� �� ��İ� ���� ���. ������ �ʿ� ���
	XMFLOAT4X4 view = Matrix4x4::Identity();
	XMFLOAT4X4 proj = Matrix4x4::Identity();

	LightType lightType;
};