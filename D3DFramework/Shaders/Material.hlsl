struct MaterialData
{
	float4x4 matTransform;
	float4   diffuseAlbedo;
	float3   specular;
	float    roughness;
	uint     diffuseMapIndex;
	uint     normalMapIndex;
	uint     heightMapIndex;
	uint     matPad0;
};

struct Material
{
	float4 diffuseAlbedo;
	float3 specular;
	float shininess;
};