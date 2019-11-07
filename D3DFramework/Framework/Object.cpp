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


//게임 객체를 로컬 x-축 방향으로 이동한다. 
void Object::MoveStrafe(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecRight = XMLoadFloat3(&GetRight());
	vecPosition = vecPosition + vecRight * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position.x, position.y, position.z);
}

//게임 객체를 로컬 y-축 방향으로 이동한다.
void Object::MoveUp(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecUp = XMLoadFloat3(&GetUp());
	vecPosition = vecPosition + vecUp * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position.x, position.y, position.z);
}

//게임 객체를 로컬 z-축 방향으로 이동한다. 
void Object::MoveForward(float distance) {
	XMVECTOR vecPosition = XMLoadFloat3(&GetPosition());
	XMVECTOR vecLook = XMLoadFloat3(&GetLook());
	vecPosition = vecPosition + vecLook * distance;

	XMFLOAT3 position;
	XMStoreFloat3(&position, vecPosition);
	SetPosition(position.x, position.y, position.z);
}

//게임 객체를 주어진 각도로 회전한다. 
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