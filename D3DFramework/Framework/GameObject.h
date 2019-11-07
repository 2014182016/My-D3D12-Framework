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

	// �ؽ�ó�� �̵�, ȸ��, ũ�⸦ �����Ѵ�.
	DirectX::XMFLOAT4X4 mTexTransform = D3DUtil::Identity4x4f();

private:
	// ���͸����� ������ ������ ������ ��� ���ۿ����� �ε���
	static int mCurrentIndex;
	// �� ���� ������Ʈ�� ��ü ��� ���ۿ� �ش��ϴ� GPU ��� ������ ����
	int mObjCBIndex = -1;

	// ������ ���ؼ� �ش� ��� ���۸� �����ؾ� �ϴ����� ���θ� ��Ÿ���� ������ �÷���
	// FrameResource���� ��ü�� ��� ���۰� �����Ƿ�, FrameResouce���� ������ �����ؾ� �Ѵ�.
	// ����, ��ü�� �ڷḦ ������ ������ �ݵ�� mNumFramesDiry = NUM_FRAME_RESOURCES��
	// �����ؾ� �Ѵ�.
	int mNumFramesDirty = NUM_FRAME_RESOURCES;
};