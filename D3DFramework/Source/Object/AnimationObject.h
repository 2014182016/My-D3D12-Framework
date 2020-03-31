#pragma once

#include <Object/GameObject.h>
#include <Framework/Animation.h>

class AnimationObject : public GameObject
{
public:
	AnimationObject(std::string&& name);
	virtual ~AnimationObject();

public:
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void DefineAnimation();

private:
	void Animate(const float deltaTime);

private:
	// 현재 애니메이션하는 시간 위치
	float animTimePos = 0.0f;

	// 메쉬를 변화시키는 뼈대 애니메이션
	BoneAnimation boneAnimation;
};