#include "../PrecompiledHeader/pch.h"
#include "Random.h"
#include <random>

float Random::GetRandomFloat(float min, float max)
{
	static std::random_device rd;
	static std::mt19937_64 mt(rd());

	// min과 max의 차이가 거의 같거나 더 작다면
	// 최소값을 그냥 반환한다.
	if (max - min < FLT_EPSILON)
		return min;

	std::uniform_real_distribution uid(min, max);

	return uid(mt);
}

XMFLOAT2 Random::GetRandomFloat2(const XMFLOAT2& min, const XMFLOAT2& max)
{
	XMFLOAT2 result;

	result.x = GetRandomFloat(min.x, max.x);
	result.y = GetRandomFloat(min.y, max.y);

	return result;
}

XMFLOAT3 Random::GetRandomFloat3(const XMFLOAT3& min, const XMFLOAT3& max)
{
	XMFLOAT3 result;

	result.x = GetRandomFloat(min.x, max.x);
	result.y = GetRandomFloat(min.y, max.y);
	result.z = GetRandomFloat(min.z, max.z);

	return result;
}

XMFLOAT4 Random::GetRandomFloat4(const XMFLOAT4& min, const XMFLOAT4& max)
{
	DirectX::XMFLOAT4 result;

	result.x = GetRandomFloat(min.x, max.x);
	result.y = GetRandomFloat(min.y, max.y);
	result.z = GetRandomFloat(min.z, max.z);
	result.w = GetRandomFloat(min.w, max.w);

	return result;
}

XMFLOAT3 Random::GetRandomNormal()
{
	DirectX::XMFLOAT3 result;

	result.x = GetRandomFloat(0.0f, 1.0f);
	result.y = GetRandomFloat(0.0f, 1.0f);
	result.z = GetRandomFloat(0.0f, 1.0f);

	return Vector3::Normalize(result);;
}