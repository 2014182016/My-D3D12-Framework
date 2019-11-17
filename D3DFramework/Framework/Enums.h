
#ifndef ENUMS_H
#define ENUMS_H

#include "pch.h"

enum class Option : int
{
	Debug = 0,
	Wireframe,
	Fullscreen,
	Fog,
	Lighting,
	Msaa
};

enum class CollisionType : int
{
	AABB = 0,
	OBB,
	Sphere,
	Complex,
	None
};

enum class LightType : std::uint32_t
{
	DirectioanlLight = 0,
	PointLight,
	SpotLight
};

// RenderLayer�� ������ ���� ������ �����Ѵ�.
enum class RenderLayer : int
{
	Opaque = 0,
	AlphaTested,
	Billborad,
	Transparent,
	Count,
};

#endif // ENUMS_H