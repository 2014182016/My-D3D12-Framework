#include "pch.h"
#include "Object.h"

using namespace DirectX;

Object::Object(std::string name) : Base(name) { }

Object::~Object() { }

void Object::Tick(float deltaTime)
{
	if (mIsWorldUpdate)
	{
		WorldUpdate();

		mIsWorldUpdate = false;
	}
}


XMMATRIX Object::GetWorld() const
{
	XMMATRIX mWorldMatrix = XMLoadFloat4x4(&mWorld);
	return mWorldMatrix;
}

//���� ��ü�� ���� x-�� ���͸� ��ȯ�Ѵ�.
XMFLOAT3 Object::GetRight() const
{
	XMFLOAT3 right(mWorld._11, mWorld._12, mWorld._13);
	XMVECTOR vecRight = XMLoadFloat3(&right);
	vecRight = XMVector3Normalize(vecRight);
	XMStoreFloat3(&right, vecRight);
	return right;
}

//���� ��ü�� ���� y-�� ���͸� ��ȯ�Ѵ�. 
XMFLOAT3 Object::GetUp() const
{
	XMFLOAT3 up(mWorld._21, mWorld._22, mWorld._23);
	XMVECTOR vecUp = XMLoadFloat3(&up);
	vecUp = XMVector3Normalize(vecUp);
	XMStoreFloat3(&up, vecUp);
	return up;
}

//���� ��ü�� ���� z-�� ���͸� ��ȯ�Ѵ�.
XMFLOAT3 Object::GetLook() const
{
	XMFLOAT3 look(mWorld._31, mWorld._32, mWorld._33);
	XMVECTOR vecLook = XMLoadFloat3(&look);
	vecLook = XMVector3Normalize(vecLook);
	XMStoreFloat3(&look, vecLook);
	return look;
}

void Object::SetPosition(float posX, float posY, float posZ)
{
	mPosition.x = posX;
	mPosition.y = posY;
	mPosition.z = posZ;

	mIsWorldUpdate = true;
}

void Object::SetRotation(float rotX, float rotY, float rotZ)
{
	mPosition.x = rotX;
	mRotation.y = rotY;
	mRotation.z = rotZ;

	mIsWorldUpdate = true;
}

void Object::SetScale(float scaleX, float scaleY, float scaleZ)
{
	mScale.x = scaleX;
	mScale.y = scaleY;
	mScale.z = scaleZ;

	mIsWorldUpdate = true;
}

void Object::Move(float x, float y, float z)
{
	mPosition.x += x;
	mPosition.y += y;
	mPosition.z += z;

	mIsWorldUpdate = true;
}

//���� ��ü�� ���� x-�� �������� �̵��Ѵ�. 
void Object::MoveStrafe(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecRight = XMLoadFloat3(&GetRight());
	vecPosition = vecPosition + vecRight * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position);

	mIsWorldUpdate = true;
}

//���� ��ü�� ���� y-�� �������� �̵��Ѵ�.
void Object::MoveUp(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecUp = XMLoadFloat3(&GetUp());
	vecPosition = vecPosition + vecUp * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position);

	mIsWorldUpdate = true;
}

//���� ��ü�� ���� z-�� �������� �̵��Ѵ�. 
void Object::MoveForward(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecLook = XMLoadFloat3(&GetLook());
	vecPosition = vecPosition + vecLook * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position);

	mIsWorldUpdate = true;
}

//���� ��ü�� �־��� ������ ȸ���Ѵ�. 
void Object::Rotate(float pitch, float yaw, float roll)
{
	mRotation.x += XMConvertToRadians(pitch);
	mRotation.y += XMConvertToRadians(yaw);
	mRotation.z += XMConvertToRadians(roll);

	mIsWorldUpdate = true;
}

void Object::Rotate(XMFLOAT3* axis, float angle)
{
	XMMATRIX rotation = XMMatrixRotationAxis(XMLoadFloat3(axis), XMConvertToRadians(angle));
	XMMATRIX world = XMMatrixMultiply(rotation, XMLoadFloat4x4(&mWorld));
	XMStoreFloat4x4(&mWorld, world);
}

void Object::WorldUpdate()
{
	XMMATRIX translation = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(mRotation.x, mRotation.y, mRotation.z);
	XMMATRIX scailing = XMMatrixScaling(mScale.x, mScale.y, mScale.z);

	// S * R * T������ ���Ͽ� world�� �����Ѵ�.
	XMMATRIX world = XMMatrixMultiply(scailing, XMMatrixMultiply(rotation, translation));
	XMStoreFloat4x4(&mWorld, world);
}