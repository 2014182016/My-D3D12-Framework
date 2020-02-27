#pragma once

#include "pch.h"

class Random
{
public:
	Random() = delete;
	~Random() = delete;

public:
	static float GetRandomFloat(float min, float max);
	static DirectX::XMFLOAT2 GetRandomFloat2(const DirectX::XMFLOAT2& min, const DirectX::XMFLOAT2& max);
	static DirectX::XMFLOAT3 GetRandomFloat3(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max);
	static DirectX::XMFLOAT4 GetRandomFloat4(const DirectX::XMFLOAT4& min, const DirectX::XMFLOAT4& max);
	static DirectX::XMFLOAT3 GetRandomNormal();
};