#include "pch.h"
#include "Object.h"
#include "D3DFramework.h"

using namespace DirectX;

Object::Object(std::string name) : mName(name) { }

Object::~Object() { }

bool Object::operator==(const Object& rhs)
{
	int result = mName.compare(rhs.mName);

	if (result == 0)
		return true;
	return false;
}

bool Object::operator==(const std::string& str)
{
	if (mName.compare(str) == 0)
		return true;
	return false;
}

void Object::Destroy()
{
	std::forward_list<ObjectPtr>& allObjects = D3DFramework::GetApp()->GetAllObjects();

	const Object* myObjPtr = this;
	allObjects.remove_if([&myObjPtr](const ObjectPtr& obj1) -> bool {
		return obj1.get() == myObjPtr; });

	D3DFramework::GetApp()->ClassifyObjectLayer();
}

std::string Object::ToString() const
{
	return mName;
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

void Object::SetPosition(float x, float y, float z)
{
	mWorld._41 = x;
	mWorld._42 = y;
	mWorld._43 = z;
}

void Object::SetScale(float scaleX, float scaleY, float scaleZ)
{
	XMMATRIX world = XMMatrixScaling(scaleX, scaleY, scaleZ);
	XMStoreFloat4x4(&mWorld, world);
}


//���� ��ü�� ���� x-�� �������� �̵��Ѵ�. 
void Object::MoveStrafe(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecRight = XMLoadFloat3(&GetRight());
	vecPosition = vecPosition + vecRight * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position.x, position.y, position.z);
}

//���� ��ü�� ���� y-�� �������� �̵��Ѵ�.
void Object::MoveUp(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecUp = XMLoadFloat3(&GetUp());
	vecPosition = vecPosition + vecUp * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position.x, position.y, position.z);
}

//���� ��ü�� ���� z-�� �������� �̵��Ѵ�. 
void Object::MoveForward(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecLook = XMLoadFloat3(&GetLook());
	vecPosition = vecPosition + vecLook * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position.x, position.y, position.z);
}

//���� ��ü�� �־��� ������ ȸ���Ѵ�. 
void Object::Rotate(float pitch, float yaw, float rool) {
	XMMATRIX rotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch),
		XMConvertToRadians(yaw), XMConvertToRadians(rool));
	XMMATRIX world = XMMatrixMultiply(XMLoadFloat4x4(&mWorld), rotate);
	XMStoreFloat4x4(&mWorld, world);
}

void Object::Rotate(XMFLOAT3* axis, float angle) {
	XMMATRIX rotate = XMMatrixRotationAxis(XMLoadFloat3(axis), XMConvertToRadians(angle));
	XMMATRIX world = XMMatrixMultiply(XMLoadFloat4x4(&mWorld), rotate);
	XMStoreFloat4x4(&mWorld, world);
}