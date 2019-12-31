#include "pch.h"

namespace DirectX
{
	std::ostream& operator<<(std::ostream& os, const DirectX::XMFLOAT3& xmf)
	{
		os << xmf.x << " " << xmf.y << " " << xmf.z;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const DirectX::XMFLOAT4X4& xmf4x4f)
	{
		os << xmf4x4f._11 << " " << xmf4x4f._12 << " " << xmf4x4f._13 << " " << xmf4x4f._14 << std::endl;
		os << xmf4x4f._21 << " " << xmf4x4f._22 << " " << xmf4x4f._23 << " " << xmf4x4f._24 << std::endl;
		os << xmf4x4f._31 << " " << xmf4x4f._32 << " " << xmf4x4f._33 << " " << xmf4x4f._34 << std::endl;
		os << xmf4x4f._41 << " " << xmf4x4f._42 << " " << xmf4x4f._43 << " " << xmf4x4f._44;
		return os;
	}

	DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& lhs, const float rhs)
	{
		DirectX::XMFLOAT3 result;

		DirectX::XMVECTOR vecLhs = DirectX::XMLoadFloat3(&lhs);
		DirectX::XMVECTOR vecRhs = DirectX::XMVectorReplicate(rhs);
		DirectX::XMVECTOR vecResult = vecLhs + vecRhs;

		DirectX::XMStoreFloat3(&result, vecResult);
		return result;
	}

	DirectX::XMFLOAT3 operator*(const DirectX::XMFLOAT3& lhs, const float rhs)
	{
		DirectX::XMFLOAT3 result;

		DirectX::XMVECTOR vecLhs = DirectX::XMLoadFloat3(&lhs);
		DirectX::XMVECTOR vecRhs = DirectX::XMVectorReplicate(rhs);
		DirectX::XMVECTOR vecResult = vecLhs * vecRhs;

		DirectX::XMStoreFloat3(&result, vecResult);
		return result;
	}

	DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
	{
		DirectX::XMFLOAT3 result;

		DirectX::XMVECTOR vecLhs = DirectX::XMLoadFloat3(&lhs);
		DirectX::XMVECTOR vecRhs = DirectX::XMLoadFloat3(&rhs);
		DirectX::XMVECTOR vecResult = vecLhs + vecRhs;

		DirectX::XMStoreFloat3(&result, vecResult);
		return result;
	}

	DirectX::XMFLOAT3 operator-(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
	{
		DirectX::XMFLOAT3 result;

		DirectX::XMVECTOR vecLhs = DirectX::XMLoadFloat3(&lhs);
		DirectX::XMVECTOR vecRhs = DirectX::XMLoadFloat3(&rhs);
		DirectX::XMVECTOR vecResult = vecLhs - vecRhs;

		DirectX::XMStoreFloat3(&result, vecResult);
		return result;
	}

	DirectX::XMFLOAT3 operator/(const DirectX::XMFLOAT3& lhs, const float rhs)
	{
		DirectX::XMFLOAT3 result;

		DirectX::XMVECTOR vecLhs = DirectX::XMLoadFloat3(&lhs);
		DirectX::XMVECTOR vecRhs = DirectX::XMVectorReplicate(rhs);
		DirectX::XMVECTOR vecResult = vecLhs / vecRhs;

		DirectX::XMStoreFloat3(&result, vecResult);
		return result;
	}
}

float GetRandomFloat(float min, float max)
{
	static std::random_device rd;
	static std::mt19937_64 mt(rd());

	if (max - min < FLT_EPSILON)
		return min;

	std::uniform_real_distribution uid(min, max);

	return uid(mt);
}

DirectX::XMFLOAT2 GetRandomFloat2(const DirectX::XMFLOAT2& min, const DirectX::XMFLOAT2& max)
{
	DirectX::XMFLOAT2 result;

	result.x = GetRandomFloat(min.x, max.x);
	result.y = GetRandomFloat(min.y, max.y);

	return result;
}

DirectX::XMFLOAT3 GetRandomFloat3(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max)
{
	DirectX::XMFLOAT3 result;

	result.x = GetRandomFloat(min.x, max.x);
	result.y = GetRandomFloat(min.y, max.y);
	result.z = GetRandomFloat(min.z, max.z);

	return result;
}

DirectX::XMFLOAT4 GetRandomFloat4(const DirectX::XMFLOAT4& min, const DirectX::XMFLOAT4& max)
{
	DirectX::XMFLOAT4 result;

	result.x = GetRandomFloat(min.x, max.x);
	result.y = GetRandomFloat(min.y, max.y);
	result.z = GetRandomFloat(min.z, max.z);
	result.w = GetRandomFloat(min.w, max.w);

	return result;
}

DirectX::XMFLOAT3 GetRandomNormal()
{
	DirectX::XMFLOAT3 result;

	result.x = GetRandomFloat(0.0f, 1.0f);
	result.y = GetRandomFloat(0.0f, 1.0f);
	result.z = GetRandomFloat(0.0f, 1.0f);

	DirectX::XMVECTOR vecResult = DirectX::XMLoadFloat3(&result);
	vecResult = DirectX::XMVector3Normalize(vecResult);
	DirectX::XMStoreFloat3(&result, vecResult);

	return result;
}