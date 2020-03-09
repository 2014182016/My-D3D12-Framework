
#ifndef ENUMERATION_H
#define ENUMERATION_H

enum class Option : int
{
	Debug_Collision = 0,
	Debug_Octree,
	Debug_Light,
	Debug_GBuffer,
	Wireframe,
	Fullscreen,
	Fog,
	Lighting,
	Msaa,
	Count,
};

enum class FogType : int
{
	Linear,
	Exponential,
	Exponential2,
};


enum class CollisionType : int
{
	None = 0,
	AABB,
	OBB,
	Sphere,
	Point
};

enum class LightType : int
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
	Transparent,
	Sky,
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

enum class RpCommon : int
{
	Object = 0,
	Pass,
	Light,
	Material,
	Texture,
	ShadowMap,
	GBuffer,
	Ssao,
	Count
};

enum class RpSsao : int
{
	SsaoCB = 0,
	Pass,
	Constants,
	BufferMap,
	SsaoMap,
	Count
};

enum class RpParticleGraphics : int
{
	ParticleCB = 0,
	Pass,
	Buffer,
	Texture,
	Count
};

enum class RpParticleCompute : int
{
	ParticleCB = 0,
	Counter,
	Uav,
	Count
};

#endif // ENUMERATION_H