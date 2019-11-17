#include "pch.h"
#include "Material.h"

using namespace DirectX;

int Material::mCurrentIndex = 0;

Material::Material(std::string name) : Base(name)
{
	mMatCBIndex = mCurrentIndex++;
}

Material::~Material() { }

XMMATRIX Material::GetMaterialTransform() const
{
	XMMATRIX mMatTransformMatrix = XMLoadFloat4x4(&mMatTransform);
	return mMatTransformMatrix;
}

void Material::SetDiffuse(float r, float g, float b, float a)
{
	mDiffuseAlbedo.x = r;
	mDiffuseAlbedo.y = g;
	mDiffuseAlbedo.z = b;
	mDiffuseAlbedo.w = a;
}

void Material::SetSpecular(float r, float g, float b)
{
	mSpecular.x = r;
	mSpecular.y = g;
	mSpecular.z = b;
}