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
프레임워크에 사용되는 객체들의 기반이 되는 클래스
*/
class Component
{
public:
	Component(std::string&& name);
	virtual ~Component();

public:
	// UID로 두 객체가 같은지 확인
	bool operator==(const Component& rhs);
	// 이름으로 두 객체가 같은지 확인
	bool operator==(const std::string& str);
	// UID와 이름을 합쳐 string으로 반환한다.
	std::string ToString() const;

public:
	// 객체를 시작할 때, 할 행동으 정의한다.
	virtual void BeginPlay() { };
	// 매 프레임 객체가 할 행동을 정의한다.
	virtual void Tick(float deltaTime) { };
	// 객체가 파괴될 때, 행동을 정의한다.
	virtual void Destroy() { };

public:
	// 해당 객체가 업데이트되었는지 확인한다.
	bool IsUpdate() const;
	// 해당 객체를 NUM_FRAME_RESOURCES값만큼 업데이트한다. 
	void UpdateNumFrames();
	// frame값을 1줄임으로써 프레임이 지나간 것을 표현한다.
	void DecreaseNumFrames();

	std::string GetName() const;
	UINT64 GetUID() const;

private:
	static inline UINT64 currUID = 0;

private:
	std::string mName;

	// 객체를 판별하기 위한 유일한 ID값
	UINT64 uid;

	// 재질이 변해서 해당 상수 버퍼를 갱신해야 하는지의 여부를 나타내는 더러움 플래그
	// FrameResource마다 물체의 상수 버퍼가 있으므로, FrameResouce마다 갱신을 적용해야 한다.
	// 따라서, 물체의 자료를 수정할 때에는 반드시 mNumFramesDiry = NUM_FRAME_RESOURCES을
	// 적용해야 한다.
	INT32 mNumFramesDirty = NUM_FRAME_RESOURCES;
};