//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "../PrecompiledHeader/pch.h"
#include "Camera.h"

Camera::Camera()
{
	SetLens(0.25f * XM_PI, 1.0f, 1.0f, 1000.0f);
}

Camera::~Camera()
{
}

XMVECTOR Camera::GetPosition()const
{
	return XMLoadFloat3(&position);
}

XMFLOAT3 Camera::GetPosition3f()const
{
	return position;
}

void Camera::SetPosition(const float x, const float y, const float z)
{
	listener->SetPosition(x, y, z, DS3D_IMMEDIATE);
	position = { x,y,z };

	viewDirty = true;
}

void Camera::SetPosition(const XMFLOAT3& v)
{
	listener->SetPosition(v.x, v.y, v.z, DS3D_IMMEDIATE);
	position = v;

	viewDirty = true;
}

XMVECTOR Camera::GetRight()const
{
	return XMLoadFloat3(&right);
}

XMFLOAT3 Camera::GetRight3f()const
{
	return right;
}

XMVECTOR Camera::GetUp()const
{
	return XMLoadFloat3(&up);
}

XMFLOAT3 Camera::GetUp3f()const
{
	return up;
}

XMVECTOR Camera::GetLook()const
{
	return XMLoadFloat3(&look);
}

XMFLOAT3 Camera::GetLook3f()const
{
	return look;
}

float Camera::GetNearZ()const
{
	return nearZ;
}

float Camera::GetFarZ()const
{
	return farZ;
}

float Camera::GetAspect()const
{
	return aspect;
}

float Camera::GetFovY()const
{
	return fovY;
}

float Camera::GetFovX()const
{
	float halfWidth = 0.5f*GetNearWindowWidth();
	return 2.0f*atan(halfWidth / nearZ);
}

float Camera::GetNearWindowWidth()const
{
	return aspect * nearWindowHeight;
}

float Camera::GetNearWindowHeight()const
{
	return nearWindowHeight;
}

float Camera::GetFarWindowWidth()const
{
	return aspect * farWindowHeight;
}

float Camera::GetFarWindowHeight()const
{
	return farWindowHeight;
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	this->fovY = XMConvertToRadians(fovY);
	this->aspect = aspect;
	nearZ = zn;
	farZ = zf;

	nearWindowHeight = 2.0f * nearZ * tanf( 0.5f*fovY );
	farWindowHeight  = 2.0f * farZ * tanf( 0.5f*fovY );

	proj = Matrix4x4::PerspectiveFovLH(this->fovY, aspect, nearZ, farZ);
	BoundingFrustum::CreateFromMatrix(camFrustum, GetProj());
}

void Camera::LookAt(const DirectX::FXMVECTOR& pos, const DirectX::FXMVECTOR& target, const DirectX::FXMVECTOR& worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&position, pos);
	XMStoreFloat3(&look, L);
	XMStoreFloat3(&right, R);
	XMStoreFloat3(&up, U);

	viewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);

	viewDirty = true;
}

XMMATRIX Camera::GetView()const
{
	return XMLoadFloat4x4(&view);
}

XMMATRIX Camera::GetProj()const
{
	return XMLoadFloat4x4(&proj);
}


XMFLOAT4X4 Camera::GetView4x4f()const
{
	assert(!viewDirty);
	return view;
}

XMFLOAT4X4 Camera::GetProj4x4f()const
{
	return proj;
}

void Camera::Strafe(const float d)
{
	float dist = d * cameraWalkSpeed;

	// position += dist * right
	XMVECTOR s = XMVectorReplicate(dist);
	XMVECTOR r = XMLoadFloat3(&right);
	XMVECTOR p = XMLoadFloat3(&position);
	XMStoreFloat3(&position, XMVectorMultiplyAdd(s, r, p));

	SetPosition(position);
}

void Camera::Walk(const float d)
{
	float dist = d * cameraWalkSpeed;

	// position += dist * look
	XMVECTOR s = XMVectorReplicate(dist);
	XMVECTOR l = XMLoadFloat3(&look);
	XMVECTOR p = XMLoadFloat3(&position);
	XMStoreFloat3(&position, XMVectorMultiplyAdd(s, l, p));

	SetPosition(position);
}

void Camera::Pitch(const float angle)
{
	float radians = DirectX::XMConvertToRadians(cameraRotateSpeed * angle);
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&right), radians);

	up = Vector3::TransformNormal(up, R);
	look = Vector3::TransformNormal(look, R);

	viewDirty = true;
}

void Camera::RotateY(const float angle)
{
	float radians = DirectX::XMConvertToRadians(cameraRotateSpeed * angle);
	XMMATRIX R = XMMatrixRotationY(radians);

	right = Vector3::TransformNormal(right, R);
	up = Vector3::TransformNormal(up, R);
	look = Vector3::TransformNormal(look, R);

	viewDirty = true;
}

void Camera::UpdateViewMatrix()
{
	if(viewDirty)
	{
		XMVECTOR R = XMLoadFloat3(&right);
		XMVECTOR U = XMLoadFloat3(&up);
		XMVECTOR L = XMLoadFloat3(&look);
		XMVECTOR P = XMLoadFloat3(&position);

		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));
		R = XMVector3Cross(U, L);

		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&right, R);
		XMStoreFloat3(&up, U);
		XMStoreFloat3(&look, L);

		view(0, 0) = right.x;
		view(1, 0) = right.y;
		view(2, 0) = right.z;
		view(3, 0) = x;

		view(0, 1) = up.x;
		view(1, 1) = up.y;
		view(2, 1) = up.z;
		view(3, 1) = y;

		view(0, 2) = look.x;
		view(1, 2) = look.y;
		view(2, 2) = look.z;
		view(3, 2) = z;

		view(0, 3) = 0.0f;
		view(1, 3) = 0.0f;
		view(2, 3) = 0.0f;
		view(3, 3) = 1.0f;

		viewDirty = false;
	}
}

BoundingFrustum Camera::GetWorldCameraBounding() const
{
	XMMATRIX view = GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	BoundingFrustum worldFrustrum;
	camFrustum.Transform(worldFrustrum, invView);

	return worldFrustrum;
}

void Camera::SetListener(IDirectSound3DListener8* listener)
{ 
	this->listener = listener;
}