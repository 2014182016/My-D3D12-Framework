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
	// ���� �ð��� ù ��°�� Ű ������ �ð����� �۴ٸ�
	// ���� ó�� Ű �������� ����Ѵ�.
	if (t <= keyframes.front().timePos)
	{
		XMVECTOR S = XMLoadFloat3(&keyframes.front().scale);
		XMVECTOR P = XMLoadFloat3(&keyframes.front().translation);
		XMVECTOR Q = XMLoadFloat4(&keyframes.front().rotationQuat);

		XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	// ���� �ð��� ������ ������ �ð����� ũ�ٸ�
	// ���� ������ Ű �������� ����Ѵ�.
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
		// ���� �ð��� ù ��° Ű �����Ӱ� ������ Ű ������ ���̿� ��ġ�Ѵ�.
		// ���� �� ���̿� ��ġ�ϴ� �������� ã�� �����Ѵ�.
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

	// �� Ŭ���� ��� ���븦 �־��� �ð�(����)�� �°� �����Ѵ�.
	auto clip = animations.find(clipName);
	clip->second.Interpolate(timePos, toParentTransforms);

	// ��� ���뱸���� �����鼭 ��� ���븦 �Ѹ� �������� ��ȯ�Ѵ�.
	std::vector<XMFLOAT4X4> toRootTransforms(numBones);

	// �Ѹ��� �ε����� 0�̴�. �Ѹ� ���뿡�� �θ� �����Ƿ�
	// �Ѹ� ������ �Ѹ� ��ȯ�� �׳� �ڽ��� ���� ���� ��ȯ�̴�.
	toRootTransforms[0] = toParentTransforms[0];

	// ���� �ڽ� ������� �Ѹ� ��ȯ���� ���Ѵ�.
	for (UINT i = 1; i < numBones; ++i)
	{
		int parentIndex = boneHierarchy[i];

		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);
		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	// ���� ������ ��ȯ�� ���� ���ؼ� ���� ��ȯ�� ���Ѵ�.
	for (UINT i = 0; i < numBones; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&boneOffsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);

		XMStoreFloat4x4(&finalTransforms[i], XMMatrixTranspose(finalTransform));
	}
}