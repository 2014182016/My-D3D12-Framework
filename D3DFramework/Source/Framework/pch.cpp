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

	DirectX::XMFLOAT3 operator/(const DirectX::XMFLOAT3& lhs, const float rhs)
	{
		DirectX::XMFLOAT3 result;

		DirectX::XMVECTOR vecLhs = DirectX::XMLoadFloat3(&lhs);
		DirectX::XMVECTOR vecRhs = DirectX::XMVectorReplicate(rhs);
		DirectX::XMVECTOR vecResult = vecLhs / vecRhs;

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

	DirectX::XMFLOAT3 operator*(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
	{
		DirectX::XMFLOAT3 result;

		DirectX::XMVECTOR vecLhs = DirectX::XMLoadFloat3(&lhs);
		DirectX::XMVECTOR vecRhs = DirectX::XMLoadFloat3(&rhs);
		DirectX::XMVECTOR vecResult = vecLhs * vecRhs;

		DirectX::XMStoreFloat3(&result, vecResult);
		return result;
	}

	bool operator==(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
	{
		DirectX::XMVECTOR vecLhs = DirectX::XMLoadFloat3(&lhs);
		DirectX::XMVECTOR vecRhs = DirectX::XMLoadFloat3(&rhs);

		return XMVector3Equal(vecLhs, vecRhs);
	}
}