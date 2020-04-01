#pragma once

#include "Component.h"
#include "../Framework/Vector.h"

/*
물체의 재질을 표현하는 클래스
*/
class Material : public Component
{
public:
	Material(std::string&& name);
	virtual ~Material();
	
public:
	// UV를 해당 값으로 설정한다.
	void SetUV(float u, float v);
	// UV를 해당 값만큼 이동시킨다.
	void Move(float u, float v);
	// 머터리얼을 반시계방향으로 degree만큼 회전시킨다.
	void Rotate(float degree);
	// 머터리얼의 크기를 줄이거나 키운다.
	void SetScale(float scaleU, float scaleV);

	XMMATRIX GetMaterialTransform() const;
	XMFLOAT4X4 GetMaterialTransform4x4f() const;
	UINT32 GetMaterialIndex() const;

public:
	// 디퓨즈맵 텍스처를 위한 SRV 힙에서의 인덱스
	INT32 diffuseMapIndex = -1;
	// 노말맵 텍스처를 위한 SRV 힙에서의 인덱스
	INT32 normalMapIndex = -1;
	// 하이트맵 텍스처를 위한 SRV 힙에서의 인덱스
	INT32 heightMapIndex = -1;

	// 셰이딩에 사용되는 재질 상수 버퍼 자료
	XMFLOAT4 diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 specular = { 0.01f, 0.01f, 0.01f };
	float roughness = 0.25f;

protected:
	XMFLOAT4X4 matTransform = Matrix4x4::Identity();

private:
	// 머터리얼을 생성할 때마다 삽입할 상수 버퍼에서의 인덱스
	static inline UINT32 currentMaterialIndex = 0;

private:
	// 이 머터리얼에 일치하는 상수 버퍼에서의 인덱스
	UINT32 materialIndex = -1;
};
