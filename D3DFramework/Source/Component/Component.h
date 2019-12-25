#pragma once

#include "pch.h"

class Component
{
public:
	Component(std::string&& name);
	virtual ~Component();

public:
	// �̸��� ������� ������ Ȯ��
	virtual bool operator==(const Component& rhs);
	virtual bool operator==(const std::string& str);

	// �� ��ü�� ���� ������ string_view�� �����ִ� �Լ�
	virtual std::string ToString() const;

public:
	virtual void BeginPlay() { };
	virtual void Tick(float deltaTime) { };
	virtual void Destroy() { };

	bool IsUpdate() const;
	void UpdateNumFrames();
	void DecreaseNumFrames();

public:
	inline std::string GetName() const { return mName; }
	inline long GetUID() const { return mUID; }

private:
	static inline long currUID = 0;
	std::string mName;
	long mUID;

	// ������ ���ؼ� �ش� ��� ���۸� �����ؾ� �ϴ����� ���θ� ��Ÿ���� ������ �÷���
	// FrameResource���� ��ü�� ��� ���۰� �����Ƿ�, FrameResouce���� ������ �����ؾ� �Ѵ�.
	// ����, ��ü�� �ڷḦ ������ ������ �ݵ�� mNumFramesDiry = NUM_FRAME_RESOURCES��
	// �����ؾ� �Ѵ�.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;
};