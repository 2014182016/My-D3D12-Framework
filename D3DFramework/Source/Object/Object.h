#pragma once

#include <Component/Component.h>

/*
공간 상에서 움직일 수 있는 물체
*/
class Object : public Component
{
public:
	Object(std::string&& name);
	virtual ~Object();

public:
	virtual void Tick(float deltaTime) override;
	virtual void Destroy() override;

	// 물체가 움직일 때, 업데이트 해야할 것을 오버라이딩하여 작성한다.
	virtual void CalculateWorld();

public:
	void Move(const float x, const float y, const float z);
	void Move(const XMFLOAT3& distance);
	void MoveStrafe(const float distance);
	void MoveUp(const float distance);
	void MoveForward(const float distance);
	void Rotate(const float pitch, const float yaw, const float roll);

	//게임 객체의 로컬 x-축 벡터를 반환한다.
	XMFLOAT3 GetLook() const;
	//게임 객체의 로컬 y-축 벡터를 반환한다. 
	XMFLOAT3 GetUp() const;
	//게임 객체의 로컬 z-축 벡터를 반환한다.
	XMFLOAT3 GetRight() const;

	void SetPosition(const float posX, const float posY, const float posZ);
	void SetPosition(const XMFLOAT3& pos);
	void SetRotation(const float rotX, const float rotY, const float rotZ);
	void SetRotation(const XMFLOAT3& rotation);
	void SetScale(const float scaleX, const float scaleY, const float scaleZ);
	void SetScale(const XMFLOAT3& scale);

	XMFLOAT3 TransformWorldToLocal(const XMFLOAT3& pos) const;
	XMFLOAT3 TransformLocalToWorld(const XMFLOAT3& pos) const;

	XMMATRIX GetWorld() const;
	XMMATRIX GetWorldWithoutScailing() const;
	XMVECTOR GetAxis(const INT32 index) const;

	XMFLOAT4X4 GetWorld4x4f() const;
	XMFLOAT4X4 GetWorldWithoutScailing4x4f() const;

	XMFLOAT3 GetPosition() const;
	XMFLOAT3 GetRotation() const;
	XMFLOAT3 GetScale() const;

	bool GetIsDestroyesd() const;

	// 오브젝트가 실질적으로 움직였는지 여부를 알아낸다.
	// 단, Tick함수가 불리기 이전에 불러야 유효하다.
	bool GetIsWorldUpdate() const;

protected:
	// 세계 공간을 기준으로 물체의 지역 공간을 서술하는 세계 행렬
	// 이 행렬은 세계 공간 안에서의 물체의 위치, 방향, 크기를 결정한다.
	// 하나의 월드 행렬을 조합할 때, Scale * Rotation * Translation 순서로 적용된다.
	XMFLOAT4X4 world = Matrix4x4::Identity();
	XMFLOAT4X4 worldNoScailing = Matrix4x4::Identity();

	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	bool isWorldUpdate = true;
	bool isDestroyed = false;
};