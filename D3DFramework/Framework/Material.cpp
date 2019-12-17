#include "pch.h"
#include "Material.h"

using namespace DirectX;

Material::Material(std::string&& name) : Base(std::move(name))
{
	mMatCBIndex = mCurrentIndex++;
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

void Material::Rotate(float rotationU, float rotationV)
{
	XMStoreFloat4x4(&mMatTransform, XMMatrixRotationRollPitchYaw(rotationU, rotationV, 0.0f));
	UpdateNumFrames();
}

void Material::Scale(float scaleU, float scaleV)
{
	XMStoreFloat4x4(&mMatTransform, XMMatrixScaling(scaleU, scaleV, 0.0f));
	UpdateNumFrames();
}