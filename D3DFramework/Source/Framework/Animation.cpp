#include "pch.h"
#include "Animation.h"

using namespace DirectX;

float BoneAnimation::GetStartTime()const
{
	return mKeyframes.front().TimePos;
}

float BoneAnimation::GetEndTime()const
{
	float f = mKeyframes.back().TimePos;

	return f;
}

void BoneAnimation::Interpolate(float t, XMFLOAT4X4& M)const
{
	if (t <= mKeyframes.front().TimePos)
	{
		XMVECTOR S = XMLoadFloat3(&mKeyframes.front().mScale);
		XMVECTOR P = XMLoadFloat3(&mKeyframes.front().mTranslation);
		XMVECTOR Q = XMLoadFloat4(&mKeyframes.front().mRotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else if (t >= mKeyframes.back().TimePos)
	{
		XMVECTOR S = XMLoadFloat3(&mKeyframes.back().mScale);
		XMVECTOR P = XMLoadFloat3(&mKeyframes.back().mTranslation);
		XMVECTOR Q = XMLoadFloat4(&mKeyframes.back().mRotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else
	{
		for (UINT i = 0; i < mKeyframes.size() - 1; ++i)
		{
			if (t >= mKeyframes[i].TimePos && t <= mKeyframes[i + 1].TimePos)
			{
				float lerpPercent = (t - mKeyframes[i].TimePos) / (mKeyframes[i + 1].TimePos - mKeyframes[i].TimePos);

				XMVECTOR s0 = XMLoadFloat3(&mKeyframes[i].mScale);
				XMVECTOR s1 = XMLoadFloat3(&mKeyframes[i + 1].mScale);

				XMVECTOR p0 = XMLoadFloat3(&mKeyframes[i].mTranslation);
				XMVECTOR p1 = XMLoadFloat3(&mKeyframes[i + 1].mTranslation);

				XMVECTOR q0 = XMLoadFloat4(&mKeyframes[i].mRotationQuat);
				XMVECTOR q1 = XMLoadFloat4(&mKeyframes[i + 1].mRotationQuat);

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

float AnimationClip::GetClipStartTime()const
{
	float t = FLT_MAX;
	for (UINT i = 0; i < mBoneAnimations.size(); ++i)
	{
		t = std::min<float>(t, mBoneAnimations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetClipEndTime()const
{
	float t = 0.0f;
	for (UINT i = 0; i < mBoneAnimations.size(); ++i)
	{
		t = std::max<float>(t, mBoneAnimations[i].GetStartTime());
	}

	return t;
}

void AnimationClip::Interpolate(float t, std::vector<XMFLOAT4X4>& boneTransforms)const
{
	for (UINT i = 0; i < mBoneAnimations.size(); ++i)
	{
		mBoneAnimations[i].Interpolate(t, boneTransforms[i]);
	}
}

UINT SkinnedData::GetBoneCount() const
{
	return (UINT)mBoneHierarchy.size();
}

float SkinnedData::GetClipStartTime(const std::string& clipName) const
{
	auto clip = mAnimations.find(clipName);
	return clip->second.GetClipStartTime();
}

float SkinnedData::GetClipEndTime(const std::string& clipName) const
{
	auto clip = mAnimations.find(clipName);
	return clip->second.GetClipEndTime();
}

void SkinnedData::Set(std::vector<int>& boneHierarchy,
	std::vector<XMFLOAT4X4>& boneOffsets,
	std::unordered_map<std::string, AnimationClip>& animations)
{
	mBoneHierarchy = boneHierarchy;
	mBoneOffsets = boneOffsets;
	mAnimations = animations;
}

void SkinnedData::GetFinalTransforms(const std::string& clipName, float timePos, std::vector<XMFLOAT4X4>& finalTransforms) const
{
	UINT numBones = (UINT)mBoneOffsets.size();

	std::vector<XMFLOAT4X4> toParentTransforms(numBones);

	// �� Ŭ���� ��� ���븦 �־��� �ð�(����)�� �°� �����Ѵ�.
	auto clip = mAnimations.find(clipName);
	clip->second.Interpolate(timePos, toParentTransforms);

	// ��� ���뱸���� �����鼭 ��� ���븦 �Ѹ� �������� ��ȯ�Ѵ�.
	std::vector<XMFLOAT4X4> toRootTransforms(numBones);

	// �Ѹ��� �ε����� 0�̴�. �Ѹ� ���뿡�� �θ� �����Ƿ�
	// �Ѹ� ������ �Ѹ� ��ȯ�� �׳� �ڽ��� ���� ���� ��ȯ�̴�.
	toRootTransforms[0] = toParentTransforms[0];

	// ���� �ڽ� ������� �Ѹ� ��ȯ���� ���Ѵ�.
	for (UINT i = 1; i < numBones; ++i)
	{
		int parentIndex = mBoneHierarchy[i];

		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);
		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	// ���� ������ ��ȯ�� ���� ���ؼ� ���� ��ȯ�� ���Ѵ�.
	for (UINT i = 0; i < numBones; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&mBoneOffsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);

		XMStoreFloat4x4(&finalTransforms[i], XMMatrixTranspose(finalTransform));
	}
}