#pragma once

#include "Component.h"
#include "Enumeration.h"

class Object : public Component
{
public:
	Object(std::string&& name);
	virtual ~Object();

public:
	virtual void Tick(float deltaTime) override;
	virtual void Destroy() override;

	// ��ü�� ������ ��, ������Ʈ �ؾ��� ���� �������̵��Ͽ� �ۼ��Ѵ�.
	virtual void CalculateWorld();

public:
	void Move(float x, float y, float z);
	void Move(const DirectX::XMFLOAT3& distance);
	void MoveStrafe(float distance);
	void MoveUp(float distance);
	void MoveForward(float distance);
	void Rotate(float pitch, float yaw, float roll);

	DirectX::XMFLOAT3 GetLook() const;
	DirectX::XMFLOAT3 GetUp() const;
	DirectX::XMFLOAT3 GetRight() const;

	void SetPosition(float posX, float posY, float posZ);
	void SetPosition(const DirectX::XMFLOAT3& pos);
	void SetRotation(float rotX, float rotY, float rotZ);
	void SetRotation(const DirectX::XMFLOAT3& rotation);
	void SetScale(float scaleX, float scaleY, float scaleZ);
	void SetScale(const DirectX::XMFLOAT3& scale);

	DirectX::XMFLOAT3 TransformWorldToLocal(const DirectX::XMFLOAT3& pos) const;
	DirectX::XMFLOAT3 TransformLocalToWorld(const DirectX::XMFLOAT3& pos) const;

	DirectX::XMMATRIX GetWorld() const;
	DirectX::XMMATRIX GetWorldWithoutScailing() const;

	DirectX::XMVECTOR GetAxis(int index) const;

public:
	inline DirectX::XMFLOAT4X4 GetWorld4x4f() const { return mWorld; }
	inline DirectX::XMFLOAT4X4 GetWorldWithoutScailing4x4f() const { return mWorldNoScailing; }

	inline DirectX::XMFLOAT3 GetPosition() const { return mPosition; }
	inline DirectX::XMFLOAT3 GetRotation() const { return mRotation; }
	inline DirectX::XMFLOAT3 GetScale() const { return mScale; }

	inline bool GetIsDestroyesd() const { return mIsDestroyed; }

	// ������Ʈ�� ���������� ���������� ���θ� �˾Ƴ���.
	// ��, Tick�Լ��� �Ҹ��� ������ �ҷ��� ��ȿ�ϴ�.
	inline bool GetIsWorldUpdate() const { return mIsWorldUpdate; }

protected:
	// ���� ������ �������� ��ü�� ���� ������ �����ϴ� ���� ���
	// �� ����� ���� ���� �ȿ����� ��ü�� ��ġ, ����, ũ�⸦ �����Ѵ�.
	// �ϳ��� ���� ����� ������ ��, Scale * Rotation * Translation ������ ����ȴ�.
	DirectX::XMFLOAT4X4 mWorld = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mWorldNoScailing = DirectX::Identity4x4f();

	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRotation = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mScale = { 1.0f, 1.0f, 1.0f };

	bool mIsWorldUpdate = true;
	bool mIsDestroyed = false;
};