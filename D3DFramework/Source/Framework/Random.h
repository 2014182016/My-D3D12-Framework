#pragma once

#include <Framework/Vector.h>

/*
랜덤한 수를 반환하는 클래스
*/
class Random
{
public:
	Random() = delete;
	~Random() = delete;

public:
	static float GetRandomFloat(float min, float max);
	static XMFLOAT2 GetRandomFloat2(const XMFLOAT2& min, const XMFLOAT2& max);
	static XMFLOAT3 GetRandomFloat3(const XMFLOAT3& min, const XMFLOAT3& max);
	static XMFLOAT4 GetRandomFloat4(const XMFLOAT4& min, const XMFLOAT4& max);
	static XMFLOAT3 GetRandomNormal();
};