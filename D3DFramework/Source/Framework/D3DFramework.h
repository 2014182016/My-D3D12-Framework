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
	virtual void CreateDescriptorHeaps(UINT textureNum, UINT shadowMapNum, UINT particleNum) override;

public:
	void RenderObject(ID3D12GraphicsCommandList* cmdList, class Renderable* obj,
		D3D12_GPU_VIRTUAL_ADDRESS startAddress, DirectX::BoundingFrustum* frustum = nullptr) const;
	void RenderObjects(ID3D12GraphicsCommandList* cmdList, const std::list<std::shared_ptr<class Renderable>>& list,
		D3D12_GPU_VIRTUAL_ADDRESS startAddress, DirectX::BoundingFrustum* frustum = nullptr, UINT threadIndex = 0, UINT threadNum = 1) const;
	void RenderActualObjects(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum = nullptr);
	void WorkerThread(UINT threadIndex);

public:
	bool Picking(struct HitInfo& hitInfo, const int screenX, const int screenY,
		const float distance = 1000.0f, const bool isMeshCollision = false) const;
	class GameObject* FindGameObject(std::string name);
	class GameObject* FindGameObject(long uid);

public:
	void DrawDebugOctree();
	void DrawDebugCollision();
	void DrawDebugLight();

public:
	inline static D3DFramework* GetInstance() { return instance; }
	inline class Camera* GetCamera() const { return mCamera.get(); }

private:
	void InitFramework();
	void CreateObjects();
	void CreateLights();
	void CreateWidgets(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void CreateParticles();
	void CreateTerrain();
	void CreateFrameResources(ID3D12Device* device);
	void CreateThreads();
	void CreateTerrainStdDevAndNormalMap();

	void UpdateObjectBuffer(float deltaTime);
	void UpdateLightBuffer(float deltaTime);
	void UpdateMaterialBuffer(float deltaTime);
	void UpdateMainPassBuffer(float deltaTime);
	void UpdateWidgetBuffer(float deltaTime);
	void UpdateParticleBuffer(float deltaTime);
	void UpdateSsaoBuffer(float deltaTime);
	void UpdateTerrainBuffer(float deltaTime);
	void UpdateSsrBuffer(float deltaTime);
	void UpdateObjectBufferPool();

	void WireframePass(ID3D12GraphicsCommandList* cmdList);
	void ShadowMapPass(ID3D12GraphicsCommandList* cmdList);
	void GBufferPass(ID3D12GraphicsCommandList* cmdList);
	void LightingPass(ID3D12GraphicsCommandList* cmdList);
	void ForwardPass(ID3D12GraphicsCommandList* cmdList);
	void PostProcessPass(ID3D12GraphicsCommandList* cmdList);

	void ParticleUpdate(ID3D12GraphicsCommandList* cmdList);
	void DrawTerrain(ID3D12GraphicsCommandList* cmdList, bool isWireframe);

	void Init(ID3D12GraphicsCommandList* cmdList);
	void MidFrame(ID3D12GraphicsCommandList* cmdList);
	void Finish(ID3D12GraphicsCommandList* cmdList);
	void SetCommonState(ID3D12GraphicsCommandList* cmdList);

private:
	static inline D3DFramework* instance = nullptr;

	std::array<std::unique_ptr<struct FrameResource>, NUM_FRAME_RESOURCES> mFrameResources;
	struct FrameResource* mCurrentFrameResource = nullptr;
	UINT mCurrentFrameResourceIndex = 0;

	std::vector<std::thread> mWorkerThread;
	std::vector<HANDLE> mWorkerBeginFrameEvents;
	std::vector<HANDLE> mWorkerFinishedFrameEvents;

	std::array<std::list<std::shared_ptr<class Renderable>>, (int)RenderLayer::Count> mRenderableObjects;
	std::list<std::shared_ptr<class GameObject>> mGameObjects;
	std::list<std::shared_ptr<class Light>> mLights;
	std::list<std::shared_ptr<class Widget>> mWidgets;
	std::list<std::shared_ptr<class Particle>> mParticles;
	std::shared_ptr<class Terrain> mTerrain;

	std::unique_ptr<struct PassConstants> mMainPassCB;
	std::array<std::unique_ptr<struct PassConstants>, LIGHT_NUM> mShadowPassCB;
	std::unique_ptr<class Camera> mCamera;
	std::unique_ptr<class Octree> mOctreeRoot;
	std::unique_ptr<class Ssao> mSsao;
	std::unique_ptr<class Ssr> mSsr;
	std::unique_ptr<class BlurFilter> mBlurFilter;
	std::unique_ptr<class D3DDebug> mD3DDebug;
	
	DirectX::BoundingFrustum mWorldCamFrustum;
};
