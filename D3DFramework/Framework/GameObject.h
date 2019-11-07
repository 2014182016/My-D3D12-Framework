#pragma once

#include "Object.h"
#include "Global.h"


class GameObject : public Object
{
public:
	GameObject(std::string name);
	virtual ~GameObject();

public:
	bool IsUpdate() const;
	void UpdateNumFrames();
	void DecreaseNumFrames();
	
public:
	DirectX::XMMATRIX GetTexTransform() const;
	inline DirectX::XMFLOAT4X4 GetTexTransform4x4f() const { return mTexTransform; }

	inline int GetObjectCBIndex() const { return mObjCBIndex; }

	inline Material* GetMaterial() const { return mMaterial; }
	inline void SetMaterial(Material* mat) { mMaterial = mat; }
	inline MeshGeometry* GetMesh() const { return mMesh; }
	inline void SetMesh(MeshGeometry* mesh) { mMesh = mesh; }

protected:
	struct Material* mMaterial = nullptr;
	struct MeshGeometry* mMesh = nullptr;

	// 텍스처의 이동, 회전, 크기를 지정한다.
	DirectX::XMFLOAT4X4 mTexTransform = D3DUtil::Identity4x4f();

private:
	// 머터리얼을 생성할 때마다 삽입할 상수 버퍼에서의 인덱스
	static int mCurrentIndex;
	// 이 게임 오브젝트의 물체 상수 버퍼에 해당하는 GPU 상수 버퍼의 색인
	int mObjCBIndex = -1;

	// 재질이 변해서 해당 상수 버퍼를 갱신해야 하는지의 여부를 나타내는 더러움 플래그
	// FrameResource마다 물체의 상수 버퍼가 있으므로, FrameResouce마다 갱신을 적용해야 한다.
	// 따라서, 물체의 자료를 수정할 때에는 반드시 mNumFramesDiry = NUM_FRAME_RESOURCES을
	// 적용해야 한다.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;
};