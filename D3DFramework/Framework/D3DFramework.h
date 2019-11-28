#pragma once

#include "D3DApp.h"
#include "Structures.h"

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

	virtual void CreateRtvAndDsvDescriptorHeaps();

public:
	class GameObject* Picking(int screenX, int screenY, float distance = 1000.0f) const;

public:
	inline static D3DFramework* GetApp() { return static_cast<D3DFramework*>(mApp); }
	inline class Camera* GetCamera() const { return mCamera.get(); }
	inline std::list<std::shared_ptr<class GameObject>>& GetGameObjects(int layerIndex) { return mGameObjects.at(layerIndex); }

private:
	void UpdateMaterialBuffer(float deltaTime);
	void UpdateMainPassCB(float deltaTime);

	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildObjects();
	void BuildLights();

	void RenderGameObjects(ID3D12GraphicsCommandList* cmdList, 
		const std::list<std::shared_ptr<class GameObject>>& gameObjects, UINT& startObjectIndex) const;

	void DebugCollision(ID3D12GraphicsCommandList* cmdList);
	void DebugOctTree(ID3D12GraphicsCommandList* cmdList);
	void DebugLight(ID3D12GraphicsCommandList* cmdList);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	std::array<std::unique_ptr<struct FrameResource>, NUM_FRAME_RESOURCES> mFrameResources;
	FrameResource* mCurrentFrameResource = nullptr;
	int mCurrentFrameResourceIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mDefaultLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mBillboardLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mDebugLayout;

	// 모든 오브젝트를 담고 있는 포워트 리스트
	std::list<std::shared_ptr<class Object>> mAllObjects;

	std::array<std::list<std::shared_ptr<class GameObject>>, (int)RenderLayer::Count> mGameObjects;
	std::list<std::shared_ptr<class Light>> mLights;

	PassConstants mMainPassCB;
	UINT objCBByteSize = 0;
	UINT passCBByteSize = 0;

	DirectX::BoundingFrustum mWorldCamFrustum;

	std::unique_ptr<class Camera> mCamera;
	std::unique_ptr<class AssetManager> mAssetManager;
	std::unique_ptr<class Octree> mOctreeRoot;
	std::unique_ptr<class D3DDebug> mDebug;
};
