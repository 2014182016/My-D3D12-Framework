#include <Framework/Animation.h>
#include <algorithm>

float BoneAnimation::GetStartTime()const
{
	return keyframes.front().timePos;
}

float BoneAnimation::GetEndTime()const
{
	return keyframes.back().timePos;
}

void BoneAnimation::Interpolate(const float t, XMFLOAT4X4& M)const
{
	// 현재 시간이 첫 번째의 키 프레임 시간보다 작다면
	// 가장 처음 키 프레임을 사용한다.
	if (t <= keyframes.front().timePos)
	{
		XMVECTOR S = XMLoadFloat3(&keyframes.front().scale);
		XMVECTOR P = XMLoadFloat3(&keyframes.front().translation);
		XMVECTOR Q = XMLoadFloat4(&keyframes.front().rotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	// 현재 시간이 마지막 프레임 시간보다 크다면
	// 가장 마지막 키 프레임을 사용한다.
	else if (t >= keyframes.back().timePos)
	{
		XMVECTOR S = XMLoadFloat3(&keyframes.back().scale);
		XMVECTOR P = XMLoadFloat3(&keyframes.back().translation);
		XMVECTOR Q = XMLoadFloat4(&keyframes.back().rotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else
	{
		// 현재 시간은 첫 번째 키 프레임과 마지막 키 프레임 사이에 위치한다.
		// 따라서 그 사이에 위치하는 프레임을 찾아 보간한다.
		for (UINT i = 0; i < keyframes.size() - 1; ++i)
		{
			if (t >= keyframes[i].timePos && t <= keyframes[i + 1].timePos)
			{
				float lerpPercent = (t - keyframes[i].timePos) / (keyframes[i + 1].timePos - keyframes[i].timePos);

				XMVECTOR s0 = XMLoadFloat3(&keyframes[i].scale);
				XMVECTOR s1 = XMLoadFloat3(&keyframes[i + 1].scale);

				XMVECTOR p0 = XMLoadFloat3(&keyframes[i].translation);
				XMVECTOR p1 = XMLoadFloat3(&keyframes[i + 1].translation);

				XMVECTOR q0 = XMLoadFloat4(&keyframes[i].rotationQuat);
				XMVECTOR q1 = XMLoadFloat4(&keyframes[i + 1].rotationQuat);

				XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
				XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
				XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

				XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));

				break;
			}
		}
	}
}

float AnimationClip::GetStartTime()const
{
	float t = FLT_MAX;
	for (UINT i = 0; i < boneAnimations.size(); ++i)
	{
		t = std::min<float>(t, boneAnimations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetEndTime()const
{
	float t = 0.0f;
	for (UINT i = 0; i < boneAnimations.size(); ++i)
	{
		t = std::max<float>(t, boneAnimations[i].GetStartTime());
	}

	return t;
}

void AnimationClip::Interpolate(float t, std::vector<XMFLOAT4X4>& boneTransforms)const
{
	for (UINT i = 0; i < boneAnimations.size(); ++i)
	{
		boneAnimations[i].Interpolate(t, boneTransforms[i]);
	}
}

UINT32 SkinnedData::GetBoneCount() const
{
	return (UINT32)boneHierarchy.size();
}

float SkinnedData::GetStartTime(const std::string& clipName) const
{
	auto clip = animations.find(clipName);
	return clip->second.GetStartTime();
}

float SkinnedData::GetEndTime(const std::string& clipName) const
{
	auto clip = animations.find(clipName);
	return clip->second.GetEndTime();
}

void SkinnedData::Set(std::vector<int>& boneHierarchy,
	std::vector<XMFLOAT4X4>& boneOffsets,
	std::unordered_map<std::string, AnimationClip>& animations)
{
	boneHierarchy = boneHierarchy;
	boneOffsets = boneOffsets;
	animations = animations;
}

void SkinnedData::GetFinalTransforms(const std::string& clipName, float timePos, std::vector<XMFLOAT4X4>& finalTransforms) const
{
	UINT numBones = (UINT)boneOffsets.size();

	std::vector<XMFLOAT4X4> toParentTransforms(numBones);

	// 이 클립의 모든 뼈대를 주어진 시간(순간)에 맞게 보간한다.
	auto clip = animations.find(clipName);
	clip->second.Interpolate(timePos, toParentTransforms);

	// 골격 계통구조를 훑으면서 모든 뼈대를 뿌리 공간으로 변환한다.
	std::vector<XMFLOAT4X4> toRootTransforms(numBones);

	// 뿌리의 인덱스는 0이다. 뿌리 뼈대에는 부모가 없으므로
	// 뿌리 뼈대의 뿌리 변환은 그냥 자신의 국소 뼈대 변환이다.
	toRootTransforms[0] = toParentTransforms[0];

	// 이제 자식 뺘대들의 뿌리 변환들을 구한다.
	for (UINT i = 1; i < numBones; ++i)
	{
		int parentIndex = boneHierarchy[i];

		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);
		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	// 뼈대 오프셋 변환을 먼저 곱해서 최종 변환을 구한다.
	for (UINT i = 0; i < numBones; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&boneOffsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);

		XMStoreFloat4x4(&finalTransforms[i], XMMatrixTranspose(finalTransform));
	}
}