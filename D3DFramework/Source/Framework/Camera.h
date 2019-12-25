//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef CAMERA_H
#define CAMERA_H

#include "pch.h"

class Camera
{
public:
	Camera();
	~Camera();

public:
	// fovY�� Degree ����
	void SetLens(float fovY, float aspect, float zn, float zf);

	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	void Strafe(float d);
	void Walk(float d);

	void Pitch(float angle);
	void RotateY(float angle);

	void UpdateViewMatrix();

public:
	DirectX::XMVECTOR GetPosition()const;
	DirectX::XMFLOAT3 GetPosition3f()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& v);
	
	DirectX::XMVECTOR GetRight()const;
	DirectX::XMFLOAT3 GetRight3f()const;
	DirectX::XMVECTOR GetUp()const;
	DirectX::XMFLOAT3 GetUp3f()const;
	DirectX::XMVECTOR GetLook()const;
	DirectX::XMFLOAT3 GetLook3f()const;

	DirectX::XMMATRIX GetView()const;
	DirectX::XMFLOAT4X4 GetView4x4f()const;

	DirectX::XMMATRIX GetProj()const;
	DirectX::XMFLOAT4X4 GetProj4x4f()const;

	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;

	DirectX::BoundingFrustum GetWorldCameraBounding() const;
	inline void SetListener(IDirectSound3DListener8* listener) { mListener = listener; }

private:
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	DirectX::XMFLOAT4X4 mView = DirectX::Identity4x4f();
	DirectX::XMFLOAT4X4 mProj = DirectX::Identity4x4f();

	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	DirectX::BoundingFrustum mCamFrustum;
	IDirectSound3DListener8* mListener = nullptr;
};

#endif // CAMERA_H