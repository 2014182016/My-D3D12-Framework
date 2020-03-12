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
	class GameObject* Picking(int screenX, int screenY, float distance = 1000.0f, bool isMeshCollision = false) const;
	void WorkerThread(UINT threadIndex);

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

	void UpdateObjectBuffer(float deltaTime);
	void UpdateLightBuffer(float deltaTime);
	void UpdateMaterialBuffer(float deltaTime);
	void UpdateMainPassBuffer(float deltaTime);
	void UpdateWidgetBuffer(float deltaTime);
	void UpdateParticleBuffer(float deltaTime);
	void UpdateSsaoBuffer(float deltaTime);
	void UpdateTerrainBuffer(float deltaTime);
	void UpdateObjectBufferPool();

	void ParticleUpdate(ID3D12GraphicsCommandList* cmdList);
	void WireframePass(ID3D12GraphicsCommandList* cmdList);
	void ShadowMapPass(ID3D12GraphicsCommandList* cmdList);
	void GBufferPass(ID3D12GraphicsCommandList* cmdList);
	void SsaoPass(ID3D12GraphicsCommandList* cmdList);
	void LightingPass(ID3D12GraphicsCommandList* cmdList);
	void ForwardPass(ID3D12GraphicsCommandList* cmdList);
	void TerrainPass(ID3D12GraphicsCommandList* cmdList, bool isWireframe);

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
	
	DirectX::BoundingFrustum mWorldCamFrustum;
};
