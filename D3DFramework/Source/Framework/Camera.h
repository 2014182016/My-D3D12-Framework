//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "Vector.h"
#include <DirectXCollision.h>

struct IDirectSound3DListener;

class Camera
{
public:
	Camera();
	~Camera();

public:
	// fovY는 Degree 기준
	void SetLens(const float fovY, const float aspect, const float zn, const float zf);

	void LookAt(const FXMVECTOR& pos, const FXMVECTOR& target, const FXMVECTOR& worldUp);
	void LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);

	void Strafe(const float d);
	void Walk(const float d);

	void Pitch(const float angle);
	void RotateY(const float angle);

	// 카메라가 이동 및 회전을 하였다면 뷰 행렬을 업데이트한다.
	void UpdateViewMatrix();

public:
	XMVECTOR GetPosition()const;
	XMFLOAT3 GetPosition3f()const;
	void SetPosition(const float x, const float y, const float z);
	void SetPosition(const DirectX::XMFLOAT3& v);
	
	XMVECTOR GetRight()const;
	XMFLOAT3 GetRight3f()const;
	XMVECTOR GetUp()const;
	XMFLOAT3 GetUp3f()const;
	XMVECTOR GetLook()const;
	XMFLOAT3 GetLook3f()const;

	XMMATRIX GetView()const;
	XMFLOAT4X4 GetView4x4f()const;

	XMMATRIX GetProj()const;
	XMFLOAT4X4 GetProj4x4f()const;

	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;

	BoundingFrustum GetWorldCameraBounding() const;
	void SetListener(IDirectSound3DListener8* listener);

private:
	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 right = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 look = { 0.0f, 0.0f, 1.0f };

	XMFLOAT4X4 view = Matrix4x4::Identity();
	XMFLOAT4X4 proj = Matrix4x4::Identity();

	float nearZ = 0.0f;
	float farZ = 0.0f;
	float aspect = 0.0f;
	float fovY = 0.0f;
	float nearWindowHeight = 0.0f;
	float farWindowHeight = 0.0f;

	bool viewDirty = true;

	float cameraWalkSpeed = 200.0f;
	float cameraRotateSpeed = 0.25f;

	BoundingFrustum camFrustum;
	IDirectSound3DListener8* listener = nullptr;
};