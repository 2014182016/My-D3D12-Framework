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
	// ���� �ִϸ��̼��ϴ� �ð� ��ġ
	float animTimePos = 0.0f;

	// �޽��� ��ȭ��Ű�� ���� �ִϸ��̼�
	BoneAnimation boneAnimation;
};