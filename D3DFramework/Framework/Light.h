#pragma once

#include "pch.h"
#include "Object.h"

class Light : public Object
{
public:
	Light(std::string name);
	virtual ~Light();

public:
	// Light의 타입에 따라 LightConstants에 채워야 할 정보는 다르다.
	// 이 함수를 override하여 타입에 맞는 데이터를 채워넣어준다.
	virtual void GetLightConstants(struct LightConstants& lightConstants);

public:
	inline LightType GetLightType() const { return mLightType; }

	inline DirectX::XMFLOAT3 GetStrength() const { return mStrength; }
	inline void SetStrength(DirectX::XMFLOAT3 strength) { mStrength = strength; }
	void SetStrength(float r, float g, float b);

	inline bool GetEnabled() const { return mEnabled; }
	inline void SetEnabled(bool value) { mEnabled = value; }

protected:
	LightType mLightType;
	bool mEnabled = true;

	DirectX::XMFLOAT3 mStrength = { 0.5f, 0.5f, 0.5f };
};