
#ifndef ENUMS_H
#define ENUMS_H

#include "pch.h"

enum class Option : int
{
	Debug_Collision = 0,
	Debug_OctTree,
	Debug_Light,
	Wireframe,
	Fullscreen,
	Fog,
	Lighting,
	Msaa,
	Count,
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

// RenderLayer는 렌더할 때의 순으로 정렬한다.
enum class RenderLayer : int
{
	Opaque = 0,
	AlphaTested,
	Billborad,
	Transparent,
	Count,
};

enum class DebugType : int
{
	Debug_CollisionBox = 0,
	Debug_CollisionSphere,
	Debug_OctTree,
	Debug_Light,
	Count,
	AABB = 0,
	OBB = 0,
	Sphere = 1,
	OctTree = 2,
	Light = 3,
};

#endif // ENUMS_H