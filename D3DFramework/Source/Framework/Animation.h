#pragma once

#include "Vector.h"
#include <vector>
#include <unordered_map>

/*
�ִϸ��̼ǿ��� �� �����Ӹ��� ���ǵǴ� �����̴�.
���� �ִϸ��̼��� Ű �����ӵ��� �����Ͽ� �ִϸ��̼��� �����Ѵ�.
*/
struct Keyframe
{
	// �ش� ������ ��ġ�� ����Ű�� �ð��̴�.
	float timePos = 0.0f;

	// �ش� �����ӿ����� �����̴�.
	XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	XMFLOAT4 rotationQuat = { 0.0f, 0.0f, 0.0f, 1.0f };
};

/*
�ϳ��� ���� �ִϸ��̼��� �����Ѵ�.
����� ���� ���� Ű �������� ������
Ű �����ӵ� ���̷� �����Ͽ� ���븦 �ִϸ��̼��Ѵ�.
*/
struct BoneAnimation
{
	float GetStartTime()const;
	float GetEndTime()const;

	// �� ������ ���̸� �����Ѵ�.
	void Interpolate(const float t, DirectX::XMFLOAT4X4& M)const;

	// Ű �����ӿ� ���� ������ �����迭�� ��Ƶд�.
	std::vector<Keyframe> keyframes;
};

/*
���� ���� ���� �ִϸ��̼��� �����Ѵ�.
�ϳ��� ĳ���Ͱ� ���� ���� ���� �ִϸ��̼��� ������
�ִ� �� ó�� ���� ���븦 ���� ��ü�� �����ϴ�.
*/
struct AnimationClip
{
	float GetStartTime()const;
	float GetEndTime()const;

	// �� ������ ���̸� �����Ѵ�.
	void Interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& boneTransforms)const;

	// ���� �ִϸ��̼ǿ� ���� ������ �����迭�� ��Ƶд�.
	std::vector<BoneAnimation> boneAnimations;
};

/*
ĳ���͸� �ִϸ��̼��ϱ� ���Ͽ� �ʿ��� �ִϸ��̼� �� ����� ��Ƶд�.
��Ű�׿� �ʿ��� ��ĵ��� ����Ͽ� ��� ���ۿ� �Ѱ��־�� �Ѵ�.
*/
class SkinnedData
{
public:
	// ������ ���������� ���� �� ������ ��ȯ�Ѵ�.
	UINT32 GetBoneCount() const;

	float GetStartTime(const std::string& clipName) const;
	float GetEndTime(const std::string& clipName) const;

	// �� ���� ��������, ���� ������, �ִϸ��̼ǵ��� �����Ѵ�.
	void Set(std::vector<int>& boneHierarchy,
		std::vector<DirectX::XMFLOAT4X4>& boneOffsets,
		std::unordered_map<std::string, AnimationClip>& animations);

	// ������ ���� ������ �ִϸ��̼��� ����Ͽ� ��Ű�׿� �����
	// ��ĵ��� ����Ѵ�.
	void GetFinalTransforms(const std::string& clipName, float timePos,
		std::vector<DirectX::XMFLOAT4X4>& finalTransforms) const;

private:
	std::vector<int> boneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> boneOffsets;
	std::unordered_map<std::string, AnimationClip> animations;
};