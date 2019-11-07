#pragma once

#include "pch.h"
#include "Global.h"

class Object
{
public:
	Object(std::string name);
	virtual ~Object();

public:
	// 이름을 기반으로 같은지 확인
	virtual bool operator==(const Object& rhs);
	virtual bool operator==(const std::string& str);

public:
	virtual void BeginPlay() { }
	virtual void Tick(float deltaTime) { }
	virtual void Destroy();

	// Object를 설명하는 문자열을 리턴하는 함수
	virtual std::string ToString() const;

public:
	void MoveStrafe(float distance);
	void MoveUp(float distance);
	void MoveForward(float distance);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3 *pxmf3Axis, float fAngle);

public:
	DirectX::XMMATRIX GetWorld() const;
	inline DirectX::XMFLOAT4X4 GetWorld4x4f() const { return mWorld; }

	DirectX::XMFLOAT3 GetPosition() const { return DirectX::XMFLOAT3(mWorld._41, mWorld._42, mWorld._43); }
	DirectX::XMFLOAT3 GetLook() const;
	DirectX::XMFLOAT3 GetUp() const;
	DirectX::XMFLOAT3 GetRight() const;

	void SetPosition(float x, float y, float z);
	void SetScale(float scaleX, float scaleY, float scaleZ);

	inline std::string GetName() const { return mName; }
	inline RenderLayer GetLayer() const { return mLayer; }

	inline int GetLayerToInt() const { return (int)mLayer; }
	inline void SetLayer(RenderLayer layer) { mLayer = layer; }

protected:
	// 세계 공간을 기준으로 물체의 지역 공간을 서술하는 세계 행렬
	// 이 행렬은 세계 공간 안에서의 물체의 위치, 방향, 크기를 결정한다.
	DirectX::XMFLOAT4X4 mWorld = D3DUtil::Identity4x4f();

private:
	std::string mName;
	RenderLayer mLayer = RenderLayer::NotRender;
};

// Object와 RenderLayer를 Less 형식으로 비교하기 위한 함수 객체
struct ObjectLess
{
	bool operator()(const std::unique_ptr<Object>& obj1, const std::unique_ptr<Object>& obj2) const
	{
		return obj1.get()->GetLayerToInt() < obj2.get()->GetLayerToInt();
	}

	bool operator()(const int& i, const std::unique_ptr<Object>& obj) const
	{
		return obj.get()->GetLayerToInt() > i;
	}

	bool operator()(const std::unique_ptr<Object>& obj, const int& i) const
	{
		return obj.get()->GetLayerToInt() < i;
	}
};