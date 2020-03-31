#include <Object/AnimationObject.h>

AnimationObject::AnimationObject(std::string&& name) : GameObject(std::move(name)) { }

AnimationObject::~AnimationObject() { }

void AnimationObject::BeginPlay()
{
	__super::BeginPlay();

	DefineAnimation();
}

void AnimationObject::DefineAnimation()
{
	XMVECTOR q0 = XMQuaternionRotationAxis(
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(30.0f));
	XMVECTOR q1 = XMQuaternionRotationAxis(
		XMVectorSet(1.0f, 1.0f, 2.0f, 0.0f), XMConvertToRadians(45.0f));
	XMVECTOR q2 = XMQuaternionRotationAxis(
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(-30.0f));
	XMVECTOR q3 = XMQuaternionRotationAxis(
		XMVectorSet(1.0f, 0.0f, 1.0f, 0.0f), XMConvertToRadians(-45.0f));

	boneAnimation.keyframes.resize(5);

	boneAnimation.keyframes[0].timePos = 0.0f;
	boneAnimation.keyframes[0].translation = GetPosition();
	XMStoreFloat4(&boneAnimation.keyframes[0].rotationQuat, q0);

	boneAnimation.keyframes[1].timePos = 2.0f;
	boneAnimation.keyframes[1].translation = GetPosition();
	XMStoreFloat4(&boneAnimation.keyframes[1].rotationQuat, q1);

	boneAnimation.keyframes[2].timePos = 4.0f;
	boneAnimation.keyframes[2].translation = GetPosition();
	XMStoreFloat4(&boneAnimation.keyframes[2].rotationQuat, q2);

	boneAnimation.keyframes[3].timePos = 6.0f;
	boneAnimation.keyframes[3].translation = GetPosition();
	XMStoreFloat4(&boneAnimation.keyframes[3].rotationQuat, q3);

	boneAnimation.keyframes[4].timePos = 8.0f;
	boneAnimation.keyframes[4].translation = GetPosition();
	XMStoreFloat4(&boneAnimation.keyframes[4].rotationQuat, q0);
}

void AnimationObject::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	Animate(deltaTime);
}

void AnimationObject::Animate(const float deltaTime)
{
	animTimePos += deltaTime;
	if (animTimePos >= boneAnimation.GetEndTime())
	{
		animTimePos = 0.0f;
	}

	boneAnimation.Interpolate(animTimePos, world);

	isWorldUpdate = true;
}