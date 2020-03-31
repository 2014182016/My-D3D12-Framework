#include "../PrecompiledHeader/pch.h"
#include "Object.h"

Object::Object(std::string&& name) : Component(std::move(name)) { }

Object::~Object() { }

void Object::Destroy()
{
	isDestroyed = true;
}

void Object::Tick(float deltaTime)
{
	if (isWorldUpdate)
	{
		CalculateWorld();
		UpdateNumFrames();

		isWorldUpdate = false;
	}
}


XMMATRIX Object::GetWorld() const
{
	XMMATRIX matWorld = XMLoadFloat4x4(&world);
	return matWorld;
}

XMMATRIX Object::GetWorldWithoutScailing() const
{
	XMMATRIX matWorld = XMLoadFloat4x4(&worldNoScailing);
	return matWorld;
}

XMVECTOR Object::GetAxis(const INT32 index) const
{
	XMMATRIX matWorld = XMLoadFloat4x4(&worldNoScailing);
	return matWorld.r[index];
}

XMFLOAT3 Object::TransformWorldToLocal(const XMFLOAT3& pos) const
{
	XMVECTOR position = XMLoadFloat3(&pos);
	XMMATRIX world = XMLoadFloat4x4(&worldNoScailing);
	position = XMVector3Transform(position, XMMatrixInverse(&XMMatrixDeterminant(world), world));

	return Vector3::XMVectorToFloat3(position);
}

XMFLOAT3 Object::TransformLocalToWorld(const XMFLOAT3& pos) const
{
	XMVECTOR position = XMLoadFloat3(&pos);
	XMMATRIX world = XMLoadFloat4x4(&worldNoScailing);
	position = XMVector3Transform(position, world);

	return 	Vector3::XMVectorToFloat3(position);
}

XMFLOAT3 Object::GetRight() const
{
	XMFLOAT3 right(world._11, world._12, world._13);
	XMVECTOR vecRight = XMLoadFloat3(&right);
	vecRight = XMVector3Normalize(vecRight);
	XMStoreFloat3(&right, vecRight);
	return right;
}

XMFLOAT3 Object::GetUp() const
{
	XMFLOAT3 up(world._21, world._22, world._23);
	XMVECTOR vecUp = XMLoadFloat3(&up);
	vecUp = XMVector3Normalize(vecUp);
	XMStoreFloat3(&up, vecUp);
	return up;
}

XMFLOAT3 Object::GetLook() const
{
	XMFLOAT3 look(world._31, world._32, world._33);
	XMVECTOR vecLook = XMLoadFloat3(&look);
	vecLook = XMVector3Normalize(vecLook);
	XMStoreFloat3(&look, vecLook);
	return look;
}

void Object::SetPosition(float posX, float posY, float posZ)
{
	position.x = posX;
	position.y = posY;
	position.z = posZ;

	isWorldUpdate = true;
}

void Object::SetPosition(const XMFLOAT3& pos)
{
	position = pos;

	isWorldUpdate = true;
}

void Object::SetRotation(float rotX, float rotY, float rotZ)
{
	rotation.x = rotX;
	rotation.y = rotY;
	rotation.z = rotZ;

	isWorldUpdate = true;
}


void Object::SetRotation(const XMFLOAT3& rot)
{
	rotation = rot;

	isWorldUpdate = true;
}

void Object::SetScale(float scaleX, float scaleY, float scaleZ)
{
	scale.x = scaleX;
	scale.y = scaleY;
	scale.z = scaleZ;

	isWorldUpdate = true;
}

void Object::SetScale(const XMFLOAT3& scale)
{
	this->scale = scale;

	isWorldUpdate = true;
}

void Object::Move(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;

	isWorldUpdate = true;
}

void Object::Move(const XMFLOAT3& distance)
{
	Move(distance.x, distance.y, distance.z);
}

void Object::MoveStrafe(float distance)
{
	XMFLOAT3 dir = Vector3::Multiply(GetRight(), distance);
	position = Vector3::Add(position, dir);

	isWorldUpdate = true;
}

void Object::MoveUp(float distance) 
{
	XMFLOAT3 dir = Vector3::Multiply(GetUp(), distance);
	position = Vector3::Add(position, dir);

	isWorldUpdate = true;
}

void Object::MoveForward(float distance) 
{
	XMFLOAT3 dir = Vector3::Multiply(GetLook(), distance);
	position = Vector3::Add(position, dir);

	isWorldUpdate = true;
}

void Object::Rotate(float pitch, float yaw, float roll)
{
	rotation.x += XMConvertToRadians(pitch);
	rotation.y += XMConvertToRadians(yaw);
	rotation.z += XMConvertToRadians(roll);

	isWorldUpdate = true;
}

void Object::CalculateWorld()
{
	XMMATRIX translation = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(this->rotation.x, this->rotation.y, this->rotation.z);
	XMMATRIX scailing = XMMatrixScaling(scale.x, scale.y, scale.z);

	// S * R * T순으로 곱하여 world에 대입한다.
	XMMATRIX world = scailing * (rotation * translation);
	this->world = Matrix4x4::XMMatrixToFloat4X4(world);

	XMMATRIX worldNoScailing = rotation * translation;
	this->worldNoScailing = Matrix4x4::XMMatrixToFloat4X4(worldNoScailing);
}

XMFLOAT4X4 Object::GetWorld4x4f() const
{
	return world; 
}
XMFLOAT4X4 Object::GetWorldWithoutScailing4x4f() const 
{
	return worldNoScailing; 
}

XMFLOAT3 Object::GetPosition() const
{
	return position; 
}
XMFLOAT3 Object::GetRotation() const 
{
	return rotation; 
}
XMFLOAT3 Object::GetScale() const 
{
	return scale; 
}

bool Object::GetIsDestroyesd() const 
{
	return isDestroyed; 
}

bool Object::GetIsWorldUpdate() const 
{
	return isWorldUpdate; 
}