#pragma once

#include "pch.h"

class Component
{
public:
	Component(std::string&& name);
	virtual ~Component();

public:
	// 이름을 기반으로 같은지 확인
	virtual bool operator==(const Component& rhs);
	virtual bool operator==(const std::string& str);

	// 이 객체에 대한 설명을 string_view로 돌려주는 함수
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

	// 재질이 변해서 해당 상수 버퍼를 갱신해야 하는지의 여부를 나타내는 더러움 플래그
	// FrameResource마다 물체의 상수 버퍼가 있으므로, FrameResouce마다 갱신을 적용해야 한다.
	// 따라서, 물체의 자료를 수정할 때에는 반드시 mNumFramesDiry = NUM_FRAME_RESOURCES을
	// 적용해야 한다.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;
};