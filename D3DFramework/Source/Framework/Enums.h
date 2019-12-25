
#ifndef ENUMS_H
#define ENUMS_H

enum class Option : int
{
	Debug_Collision = 0,
	Debug_Octree,
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

enum class SoundType : int
{
	Sound2D = 0,
	Sound3D,
};

// RenderLayer는 렌더할 때의 순으로 정렬한다.
enum class RenderLayer : int
{
	Opaque = 0,
	AlphaTested,
	Billborad,
	Sky,
	Transparent,
	Count,
};

enum class DebugType : int
{
	Debug_CollisionBox = 0,
	Debug_CollisionSphere,
	Debug_Octree,
	Debug_Light,
	Count,
	AABB = 0,
	OBB = 0,
	Sphere = 1,
	Octree = 2,
	Light = 3,
};

enum class CubeMapFace : int
{
	PositiveX = 0,
	NegativeX = 1,
	PositiveY = 2,
	NegativeY = 3,
	PositiveZ = 4,
	NegativeZ = 5
};

#endif // ENUMS_H