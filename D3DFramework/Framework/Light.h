#pragma once

#include "pch.h"
#include "Object.h"

class Light : public Object
{
public:
	Light(std::string name);
	virtual ~Light();

public:
	// Light�� Ÿ�Կ� ���� LightConstants�� ä���� �� ������ �ٸ���.
	// �� �Լ��� override�Ͽ� Ÿ�Կ� �´� �����͸� ä���־��ش�.
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