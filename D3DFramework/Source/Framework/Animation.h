#pragma once

#include "Vector.h"
#include <vector>
#include <unordered_map>

/*
애니메이션에서 각 프레임마다 정의되는 정보이다.
뼈대 애니메이션이 키 프레임들을 보간하여 애니메이션을 수행한다.
*/
struct Keyframe
{
	// 해당 프레임 위치를 가리키는 시간이다.
	float timePos = 0.0f;

	// 해당 프레임에서의 정보이다.
	XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	XMFLOAT4 rotationQuat = { 0.0f, 0.0f, 0.0f, 1.0f };
};

/*
하나의 뼈대 애니메이션을 정의한다.
뼈대는 여러 개의 키 프레임을 가지고
키 프레임들 사이로 보간하여 뼈대를 애니메이션한다.
*/
struct BoneAnimation
{
	float GetStartTime()const;
	float GetEndTime()const;

	// 각 프레임 사이를 보간한다.
	void Interpolate(const float t, DirectX::XMFLOAT4X4& M)const;

	// 키 프레임에 관한 정보를 동적배열로 담아둔다.
	std::vector<Keyframe> keyframes;
};

/*
여러 개의 뼈대 애니메이션을 정의한다.
하나의 캐릭터가 여러 개의 뼈대 애니메이션을 가지고
있는 것 처럼 여러 뼈대를 가진 객체에 유용하다.
*/
struct AnimationClip
{
	float GetStartTime()const;
	float GetEndTime()const;

	// 각 프레임 사이를 보간한다.
	void Interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& boneTransforms)const;

	// 뼈대 애니메이션에 관한 정보를 동적배열로 담아둔다.
	std::vector<BoneAnimation> boneAnimations;
};

/*
캐릭터를 애니메이션하기 위하여 필요한 애니메이션 및 행렬을 담아둔다.
스키닝에 필요한 행렬들을 계산하여 상수 버퍼에 넘겨주어야 한다.
*/
class SkinnedData
{
public:
	// 뼈대의 계층구조의 수가 몇 개인지 반환한다.
	UINT32 GetBoneCount() const;

	float GetStartTime(const std::string& clipName) const;
	float GetEndTime(const std::string& clipName) const;

	// 각 뼈대 계층구조, 뼈대 오프셋, 애니메이션들을 설정한다.
	void Set(std::vector<int>& boneHierarchy,
		std::vector<DirectX::XMFLOAT4X4>& boneOffsets,
		std::unordered_map<std::string, AnimationClip>& animations);

	// 설정된 뼈대 정보와 애니메이션을 사용하여 스키닝에 사용할
	// 행렬들을 계산한다.
	void GetFinalTransforms(const std::string& clipName, float timePos,
		std::vector<DirectX::XMFLOAT4X4>& finalTransforms) const;

private:
	std::vector<int> boneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> boneOffsets;
	std::unordered_map<std::string, AnimationClip> animations;
};