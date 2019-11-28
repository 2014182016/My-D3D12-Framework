#pragma once

#include "pch.h"
#include "Base.h"
#include "Enums.h"

class Object : public Base
{
public:
	Object(std::string name);
	virtual ~Object();

public:
	virtual void BeginPlay() { }
	virtual void Tick(float deltaTime);
	virtual void Destroy();

	// ��ü�� ������ ��, ������Ʈ �ؾ��� ���� �������̵��Ͽ� �ۼ��Ѵ�.
	virtual void WorldUpdate();

public:
	void Move(float x, float y, float z);
	void Move(DirectX::XMFLOAT3 distance);
	void MoveStrafe(float distance);
	void MoveUp(float distance);
	void MoveForward(float distance);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3* axis, float angle);

public:
	DirectX::XMMATRIX GetWorld() const;
	inline DirectX::XMFLOAT4X4 GetWorld4x4f() const { return mWorld; }

	inline DirectX::XMFLOAT3 GetPosition() const { return mPosition; }
	inline DirectX::XMFLOAT3 GetRotation() const { return mRotation; }
	inline DirectX::XMFLOAT3 GetScale() const { return mScale; }

	DirectX::XMFLOAT3 GetLook() const;
	DirectX::XMFLOAT3 GetUp() const;
	DirectX::XMFLOAT3 GetRight() const;

	void SetPosition(float posX, float posY, float posZ);
	inline void SetPosition(DirectX::XMFLOAT3 pos) { mPosition = pos; }
	void SetRotation(float rotX, float rotY, float rotZ);
	inline void SetRotation(DirectX::XMFLOAT3 rotation) { mRotation = rotation; }
	void SetScale(float scaleX, float scaleY, float scaleZ);
	inline void SetScale(DirectX::XMFLOAT3 scale) { mScale = scale; }

	inline bool GetIsDestroyesd() const { return mIsDestroyed; }
	inline bool GetIsMovable() const { return mIsMovable; }
	inline void SetMovable(bool value) { mIsMovable = value; }

	// ������Ʈ�� ���������� ���������� ���θ� �˾Ƴ���.
	// ��, Tick�Լ��� �Ҹ��� ������ �ҷ��� ��ȿ�ϴ�.
	inline bool GetIsWorldUpdate() const { return mIsWorldUpdate; }

protected:
	// ���� ������ �������� ��ü�� ���� ������ �����ϴ� ���� ���
	// �� ����� ���� ���� �ȿ����� ��ü�� ��ġ, ����, ũ�⸦ �����Ѵ�.
	// �ϳ��� ���� ����� ������ ��, Scale * Rotation * Translation ������ ����ȴ�.
	DirectX::XMFLOAT4X4 mWorld = DirectX::Identity4x4f();

	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRotation = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mScale = { 1.0f, 1.0f, 1.0f };

	bool mIsWorldUpdate = true;
	bool mIsMovable = false;
	bool mIsDestroyed = false;
};