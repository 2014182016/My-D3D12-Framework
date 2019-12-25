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

public:
	void RenderGameObjects(ID3D12GraphicsCommandList* cmdList, const std::list<std::shared_ptr<class GameObject>>& gameObjects,
		UINT& startObjectIndex, DirectX::BoundingFrustum* frustum = nullptr) const;
	class GameObject* Picking(int screenX, int screenY, float distance = 1000.0f) const;

public:
	inline static D3DFramework* GetInstance() { return instance; }
	inline class Camera* GetCamera() const { return mCamera.get(); }
	inline std::list<std::shared_ptr<class GameObject>>& GetGameObjects(int layerIndex) { return mGameObjects.at(layerIndex); }

private:
	void InitFramework();

	void CreateObjects();
	void CreateLights();
	void CreateUIs();
	void CreateFrameResources();
	void CreateShadowMapResource(UINT textureNum, UINT cubeTextureNum);

	void AddGameObject(std::shared_ptr<class GameObject> object, RenderLayer renderLayer);
	void DestroyObjects();
	void UpdateObjectBufferPool();

	void UpdateLightBuffer(float deltaTime);
	void UpdateMaterialBuffer(float deltaTime);
	void UpdateMainPassCB(float deltaTime);
	void UpdateWidgetCB(float deltaTime);

	void UpdateDebugCollision(ID3D12GraphicsCommandList* cmdList);
	void UpdateDebugOctree(ID3D12GraphicsCommandList* cmdList);
	void UpdateDebugLight(ID3D12GraphicsCommandList* cmdList);
	void UpdateDebugBufferPool();

	void RenderCollisionDebug(ID3D12GraphicsCommandList* cmdList);
	void RenderWidget(ID3D12GraphicsCommandList* cmdList);

private:
	static inline D3DFramework* instance = nullptr;

	std::array<std::unique_ptr<struct FrameResource>, NUM_FRAME_RESOURCES> mFrameResources;
	struct FrameResource* mCurrentFrameResource = nullptr;
	UINT mCurrentFrameResourceIndex = 0;

	std::list<std::shared_ptr<class Object>> mAllObjects;
	std::array<std::list<std::shared_ptr<class GameObject>>, (int)RenderLayer::Count> mGameObjects;
	std::list<std::shared_ptr<class GameObject>> mActualObjects;
	std::list<std::shared_ptr<class Light>> mLights;
	std::list<std::shared_ptr<class Widget>> mWidgets;

	DirectX::BoundingSphere mSceneBounds;
	DirectX::BoundingFrustum mWorldCamFrustum;

	std::unique_ptr<struct PassConstants> mMainPassCB;
	std::unique_ptr<class ShadowMap> mShadowMap;
	std::unique_ptr<class Camera> mCamera;
	std::unique_ptr<class Octree> mOctreeRoot;
};
