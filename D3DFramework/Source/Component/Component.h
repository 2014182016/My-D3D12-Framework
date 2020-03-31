#pragma once

#include "../Framework/D3DStructure.h"
#include "../Framework/Defines.h"
#include "../Framework/Enumeration.h"
#include "../Framework/d3dx12.h"
#include <basetsd.h>
#include <wrl.h>
#include <memory>
#include <string>

/*
�����ӿ�ũ�� ���Ǵ� ��ü���� ����� �Ǵ� Ŭ����
*/
class Component
{
public:
	Component(std::string&& name);
	virtual ~Component();

public:
	// UID�� �� ��ü�� ������ Ȯ��
	bool operator==(const Component& rhs);
	// �̸����� �� ��ü�� ������ Ȯ��
	bool operator==(const std::string& str);
	// UID�� �̸��� ���� string���� ��ȯ�Ѵ�.
	std::string ToString() const;

public:
	// ��ü�� ������ ��, �� �ൿ�� �����Ѵ�.
	virtual void BeginPlay() { };
	// �� ������ ��ü�� �� �ൿ�� �����Ѵ�.
	virtual void Tick(float deltaTime) { };
	// ��ü�� �ı��� ��, �ൿ�� �����Ѵ�.
	virtual void Destroy() { };

public:
	// �ش� ��ü�� ������Ʈ�Ǿ����� Ȯ���Ѵ�.
	bool IsUpdate() const;
	// �ش� ��ü�� NUM_FRAME_RESOURCES����ŭ ������Ʈ�Ѵ�. 
	void UpdateNumFrames();
	// frame���� 1�������ν� �������� ������ ���� ǥ���Ѵ�.
	void DecreaseNumFrames();

	std::string GetName() const;
	UINT64 GetUID() const;

private:
	static inline UINT64 currUID = 0;

private:
	std::string mName;

	// ��ü�� �Ǻ��ϱ� ���� ������ ID��
	UINT64 uid;

	// ������ ���ؼ� �ش� ��� ���۸� �����ؾ� �ϴ����� ���θ� ��Ÿ���� ������ �÷���
	// FrameResource���� ��ü�� ��� ���۰� �����Ƿ�, FrameResouce���� ������ �����ؾ� �Ѵ�.
	// ����, ��ü�� �ڷḦ ������ ������ �ݵ�� mNumFramesDiry = NUM_FRAME_RESOURCES��
	// �����ؾ� �Ѵ�.
	INT32 mNumFramesDirty = NUM_FRAME_RESOURCES;
};