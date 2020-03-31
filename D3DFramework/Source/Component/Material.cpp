#include "../PrecompiledHeader/pch.h"
#include "Material.h"

Material::Material(std::string&& name) : Component(std::move(name))
{
	materialIndex = currentMaterialIndex++;
}

Material::~Material() { }


XMMATRIX Material::GetMaterialTransform() const
{
	return XMLoadFloat4x4(&matTransform);
}

XMFLOAT4X4 Material::GetMaterialTransform4x4f() const
{
	return matTransform;
}
UINT32 Material::GetMaterialIndex() const
{
	return materialIndex;
}

void Material::SetOpacity(float opacity)
{
	diffuse.w = opacity;
}

void Material::SetUV(float u, float v)
{
	matTransform(3, 0) = u;
	matTransform(3, 1) = v;

	UpdateNumFrames();
}

void Material::Move(float u, float v)
{
	float movedU = matTransform(3, 0);
	float movedV = matTransform(3, 1);

	movedU += u;
	movedV += v;

	SetUV(movedU, movedV);
}

void Material::Rotate(float degree)
{
	float theta = XMConvertToRadians(degree);
	float c = cos(theta);
	float s = sin(theta);

	if (matTransform(0, 0) < FLT_EPSILON)
		matTransform(0, 0) = 1.0f;
	if (matTransform(0, 1) < FLT_EPSILON)
		matTransform(0, 1) = 1.0f;
	if (matTransform(1, 0) < FLT_EPSILON)
		matTransform(1, 0) = 1.0f;
	if (matTransform(1, 1) < FLT_EPSILON)
		matTransform(1, 1) = 1.0f;

	matTransform(0, 0) *= c;
	matTransform(0, 1) *= -s;
	matTransform(1, 0) *= s;
	matTransform(1, 1) *= c;

	UpdateNumFrames();
}

void Material::SetScale(float scaleU, float scaleV)
{
	matTransform(0, 0) *= scaleU;
	matTransform(1, 1) *= scaleV;

	UpdateNumFrames();
}