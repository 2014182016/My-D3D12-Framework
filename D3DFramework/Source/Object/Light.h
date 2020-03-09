#pragma once

#include "Object.h"

#define SHADOW_MAP_SIZE 1024

class Light : public Object
{
public:
	Light(std::string&& name);
	virtual ~Light();

public:
	// Light의 타입에 따라 LightConstants에 채워야 할 정보는 다르다.
	// 이 함수를 override하여 타입에 맞는 데이터를 채워넣어준다.
	virtual void SetLightData(struct LightData& lightData);

public:
	void RenderSceneToShadowMap(ID3D12GraphicsCommandList* cmdList);

public:
	inline LightType GetLightType() const { return mLightType; }

	inline DirectX::XMFLOAT3 GetStrength() const { return mStrength; }
	inline void SetStrength(DirectX::XMFLOAT3 strength) { mStrength = strength; }
	void SetStrength(float r, float g, float b);
	inline DirectX::XMFLOAT2 GetShadowMapSize() const { return mShadowMapSize; }
	inline void SetShadowMapSize(DirectX::XMFLOAT2 size) { mShadowMapSize = size; }
	void SetShadowMapSize(float width, float height);

	inline float GetFalloffStart() const { return mFalloffStart; }
	inline void SetFalloffStart(float falloffStart) { mFalloffStart = falloffStart; }
	inline float GetFalloffEnd() const { return mFalloffEnd; }
	inline void SetFalloffEnd(float falloffEnd) { mFalloffEnd = falloffEnd; }
	inline float GetSpotAngle() const { return mSpotAngle; }
	inline void SetSpotAngle(float spotAngle) { mSpotAngle = spotAngle; }

	inline bool GetEnabled() const { return mEnabled; }
	inline void SetEnabled(bool value) { mEnabled = value; }

	inline DirectX::XMMATRIX GetView() const { return XMLoadFloat4x4(&mView); }
	inline DirectX::XMMATRIX GetProj() const { return XMLoadFloat4x4(&mProj); }
	inline DirectX::BoundingFrustum GetLightFrustum() const { return mLightFrustum; }

	inline class ShadowMap* GetShadowMap() { return mShadowMap.get(); }

protected:
	// NDC 공간 [-1, 1]^2을 텍스처 공간 [0, 1]^2으로 변환하는 행렬
	static inline const DirectX::XMMATRIX toTextureTransform =
		DirectX::XMMATRIX(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	std::unique_ptr<class ShadowMap> mShadowMap;
	DirectX::BoundingFrustum mLightFrustum;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	LightType mLightType;
	bool mEnabled = true;

	DirectX::XMFLOAT3 mStrength = { 0.5f, 0.5f, 0.5f };
	DirectX::XMFLOAT2 mShadowMapSize = { 50.0f, 50.0f };
	float mFalloffStart = 1.0f;
	float mFalloffEnd = 10.0f;
	float mSpotAngle = 1.0f;
};