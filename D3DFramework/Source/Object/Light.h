#pragma once

#include "Object.h"

class ShadowMap;

#define SHADOW_MAP_SIZE 1024

/*
빛의 속성을 정의하는 기본 클래스
3가지 종류의 빛은 이 클래스를 상속 받는다.
*/
class Light : public Object
{
public:
	Light(std::string&& name);
	virtual ~Light();

public:
	// Light의 타입에 따라 LightConstants에 채워야 할 정보는 다르다.
	// 이 함수를 override하여 타입에 맞는 데이터를 채워넣어준다.
	virtual void SetLightData(LightData& lightData);

public:
	// 쉐도우 맵을 그린다.
	void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList);
	ShadowMap* GetShadowMap();

	XMMATRIX GetView() const;
	XMMATRIX GetProj() const;

public:
	// 쉐이더에서 사용하기 위한 라이트 속성들
	XMFLOAT3 strength = { 0.5f, 0.5f, 0.5f };
	float falloffStart = 1.0f;
	float falloffEnd = 10.0f;
	float spotAngle = 1.0f;
	bool enabled = true;

	// 쉐도우 맵에서 투영 행렬의 크기
	XMFLOAT2 shadowMapSize = { 50.0f, 50.0f };

protected:
	// NDC 공간 [-1, 1]^2을 텍스처 공간 [0, 1]^2으로 변환하는 행렬
	static inline const XMMATRIX toTextureTransform =
		XMMATRIX(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	// 라이트 고유의 쉐도우 맵
	std::unique_ptr<ShadowMap> shadowMap = nullptr;
	// 쉐도우 맵을 그리기 위해 객체를 프러스텀 컬링하기 위해 사용
	BoundingFrustum lightFrustum;

	// 라이트 시점에서의 뷰 행렬과 투영 행렬. 쉐도우 맵에 사용
	XMFLOAT4X4 view = Matrix4x4::Identity();
	XMFLOAT4X4 proj = Matrix4x4::Identity();

	LightType lightType;
};