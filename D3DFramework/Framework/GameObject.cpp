#include "pch.h"
#include "GameObject.h"
#include "D3DFramework.h"

using namespace DirectX;

int GameObject::mCurrentIndex = 0;

GameObject::GameObject(std::string name) : Object(name) 
{
	mObjCBIndex = mCurrentIndex++;
}

GameObject::~GameObject() { }

XMMATRIX GameObject::GetTexTransform() const
{
	XMMATRIX mTexTransformMatrix = XMLoadFloat4x4(&mTexTransform);
	return mTexTransformMatrix;
}

bool GameObject::IsUpdate() const
{
	return mNumFramesDirty > 0;
}

void GameObject::UpdateNumFrames()
{
	mNumFramesDirty = NUM_FRAME_RESOURCES;
}

void GameObject::DecreaseNumFrames()
{
	--mNumFramesDirty;
}