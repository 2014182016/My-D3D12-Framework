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

//게임 객체의 로컬 x-축 벡터를 반환한다.
XMFLOAT3 Object::GetRight() const
{
	XMFLOAT3 right(mWorld._11, mWorld._12, mWorld._13);
	XMVECTOR vecRight = XMLoadFloat3(&right);
	vecRight = XMVector3Normalize(vecRight);
	XMStoreFloat3(&right, vecRight);
	return right;
}

//게임 객체의 로컬 y-축 벡터를 반환한다. 
XMFLOAT3 Object::GetUp() const
{
	XMFLOAT3 up(mWorld._21, mWorld._22, mWorld._23);
	XMVECTOR vecUp = XMLoadFloat3(&up);
	vecUp = XMVector3Normalize(vecUp);
	XMStoreFloat3(&up, vecUp);
	return up;
}

//게임 객체의 로컬 z-축 벡터를 반환한다.
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

//게임 객체를 로컬 x-축 방향으로 이동한다. 
void Object::MoveStrafe(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecRight = XMLoadFloat3(&GetRight());
	vecPosition = vecPosition + vecRight * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position);

	mIsWorldUpdate = true;
}

//게임 객체를 로컬 y-축 방향으로 이동한다.
void Object::MoveUp(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecUp = XMLoadFloat3(&GetUp());
	vecPosition = vecPosition + vecUp * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position);

	mIsWorldUpdate = true;
}

//게임 객체를 로컬 z-축 방향으로 이동한다. 
void Object::MoveForward(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecLook = XMLoadFloat3(&GetLook());
	vecPosition = vecPosition + vecLook * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position);

	mIsWorldUpdate = true;
}

//게임 객체를 주어진 각도로 회전한다. 
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

	// S * R * T순으로 곱하여 world에 대입한다.
	XMMATRIX world = XMMatrixMultiply(scailing, XMMatrixMultiply(rotation, translation));
	XMStoreFloat4x4(&mWorld, world);
}