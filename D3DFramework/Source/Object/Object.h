#pragma once

#include <Component/Component.h>

/*
���� �󿡼� ������ �� �ִ� ��ü
*/
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
	void Move(const float x, const float y, const float z);
	void Move(const XMFLOAT3& distance);
	void MoveStrafe(const float distance);
	void MoveUp(const float distance);
	void MoveForward(const float distance);
	void Rotate(const float pitch, const float yaw, const float roll);

	//���� ��ü�� ���� x-�� ���͸� ��ȯ�Ѵ�.
	XMFLOAT3 GetLook() const;
	//���� ��ü�� ���� y-�� ���͸� ��ȯ�Ѵ�. 
	XMFLOAT3 GetUp() const;
	//���� ��ü�� ���� z-�� ���͸� ��ȯ�Ѵ�.
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

	// ������Ʈ�� ���������� ���������� ���θ� �˾Ƴ���.
	// ��, Tick�Լ��� �Ҹ��� ������ �ҷ��� ��ȿ�ϴ�.
	bool GetIsWorldUpdate() const;

protected:
	// ���� ������ �������� ��ü�� ���� ������ �����ϴ� ���� ���
	// �� ����� ���� ���� �ȿ����� ��ü�� ��ġ, ����, ũ�⸦ �����Ѵ�.
	// �ϳ��� ���� ����� ������ ��, Scale * Rotation * Translation ������ ����ȴ�.
	XMFLOAT4X4 world = Matrix4x4::Identity();
	XMFLOAT4X4 worldNoScailing = Matrix4x4::Identity();

	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	bool isWorldUpdate = true;
	bool isDestroyed = false;
};