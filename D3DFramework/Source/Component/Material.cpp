#include "pch.h"
#include "Material.h"

using namespace DirectX;

Material::Material(std::string&& name) : Component(std::move(name))
{
	mMaterialIndex = currentMaterialIndex++;
}

Material::~Material() { }


XMMATRIX Material::GetMaterialTransform() const
{
	XMMATRIX mMatTransformMatrix = XMLoadFloat4x4(&mMatTransform);
	return mMatTransformMatrix;
}

void Material::SetDiffuse(float r, float g, float b)
{
	mDiffuseAlbedo.x = r;
	mDiffuseAlbedo.y = g;
	mDiffuseAlbedo.z = b;
	mDiffuseAlbedo.w = 1.0f;
}

void Material::SetSpecular(float r, float g, float b)
{
	mSpecular.x = r;
	mSpecular.y = g;
	mSpecular.z = b;
}

void Material::SetUV(float u, float v)
{
	mMatTransform(3, 0) = u;
	mMatTransform(3, 1) = v;

	UpdateNumFrames();
}

void Material::Move(float u, float v)
{
	float movedU = mMatTransform(3, 0);
	float movedV = mMatTransform(3, 1);

	movedU += u;
	movedV += v;

	SetUV(movedU, movedV);
}

void Material::Rotate(float degree)
{
	float theta = XMConvertToRadians(degree);
	float c = cos(theta);
	float s = sin(theta);

	if (mMatTransform(0, 0) < FLT_EPSILON)
		mMatTransform(0, 0) = 1.0f;
	if (mMatTransform(0, 1) < FLT_EPSILON)
		mMatTransform(0, 1) = 1.0f;
	if (mMatTransform(1, 0) < FLT_EPSILON)
		mMatTransform(1, 0) = 1.0f;
	if (mMatTransform(1, 1) < FLT_EPSILON)
		mMatTransform(1, 1) = 1.0f;

	mMatTransform(0, 0) *= c;
	mMatTransform(0, 1) *= -s;
	mMatTransform(1, 0) *= s;
	mMatTransform(1, 1) *= c;

	UpdateNumFrames();
}

void Material::Scale(float scaleU, float scaleV)
{
	mMatTransform(0, 0) *= scaleU;
	mMatTransform(1, 1) *= scaleV;

	UpdateNumFrames();
}