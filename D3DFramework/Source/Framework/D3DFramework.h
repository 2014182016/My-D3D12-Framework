#pragma once

#include "D3DApp.h"

#if defined(DEBUG) || defined(_DEBUG)
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

class D3DFramework : public D3DApp
{
public:
	D3DFramework(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName);
	D3DFramework(const D3DFramework& rhs) = delete;
	D3DFramework& operator=(const D3DFramework& rhs) = delete;
	~D3DFramework();

public:
	virtual bool Initialize() override;
	virtual void OnDestroy() override; 
	virtual void OnResize(int screenWidth, int screenHeight) override;
	virtual void Tick(float deltaTime) override;
	virtual void Render() override;
	virtual void CreateDescriptorHeaps(UINT textureNum, UINT cubeTextureNum, UINT shadowMapNum) override;

public:
	void InitFramework();
	void RenderObject(class Renderable* obj, D3D12_GPU_VIRTUAL_ADDRESS addressStarat, 
		UINT rootParameterIndex, UINT strideCBByteSize, bool visibleCheck = true, DirectX::BoundingFrustum* frustum = nullptr) const;
	void RenderObjects(const std::list<std::shared_ptr<class Renderable>>& list, D3D12_GPU_VIRTUAL_ADDRESS addressStarat,
		UINT rootParameterIndex, UINT strideCBByteSize, bool visibleCheck = true, DirectX::BoundingFrustum* frustum = nullptr) const;
	void RenderActualObjects(DirectX::BoundingFrustum* frustum = nullptr);
	class GameObject* Picking(int screenX, int screenY, float distance = 1000.0f, bool isMeshCollision = false) const;

public:
	inline static D3DFramework* GetInstance() { return instance; }
	inline class Camera* GetCamera() const { return mCamera.get(); }

private:
	void CreateObjects();
	void CreateLights();
	void CreateWidgets();
	void CreateParticles();
	void CreateFrameResources();

	void AddGameObject(std::shared_ptr<class GameObject> object, RenderLayer renderLayer);

	void UpdateObjectBuffer(float deltaTime);
	void UpdateLightBuffer(float deltaTime);
	void UpdateMaterialBuffer(float deltaTime);
	void UpdateMainPassBuffer(float deltaTime);
	void UpdateWidgetBuffer(float deltaTime);
	void UpdateParticleBuffer(float deltaTime);
	void UpdateSsaoBuffer(float deltaTime);

	void UpdateObjectBufferPool();

	void DestroyGameObjects();
	void DestroyParticles();

	void SetCommonState();

private:
	static inline D3DFramework* instance = nullptr;

	std::array<std::unique_ptr<struct FrameResource>, NUM_FRAME_RESOURCES> mFrameResources;
	struct FrameResource* mCurrentFrameResource = nullptr;
	UINT mCurrentFrameResourceIndex = 0;

	std::array<std::list<std::shared_ptr<class Renderable>>, (int)RenderLayer::Count> mRenderableObjects;
	std::list<std::shared_ptr<class GameObject>> mGameObjects;
	std::list<std::shared_ptr<class Light>> mLights;
	std::list<std::shared_ptr<class Widget>> mWidgets;
	std::list<std::shared_ptr<class Particle>> mParticles;

	std::unique_ptr<struct PassConstants> mMainPassCB;
	std::array<std::unique_ptr<struct PassConstants>, LIGHT_NUM> mShadowPassCB;
	std::unique_ptr<class Camera> mCamera;
	std::unique_ptr<class Octree> mOctreeRoot;
	std::unique_ptr<class Ssao> mSsao;

	DirectX::BoundingFrustum mWorldCamFrustum;
};
