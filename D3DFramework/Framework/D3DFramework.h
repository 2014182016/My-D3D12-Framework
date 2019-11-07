#pragma once

#include "D3DApp.h"

#if defined(DEBUG) || defined(_DEBUG)
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

typedef std::unique_ptr<class Object> ObjectPtr;
typedef std::forward_list<ObjectPtr>::iterator LayerIter;

class D3DFramework : public D3DApp
{
public:
	D3DFramework(HINSTANCE hInstance, int screenWidth, int screenHeight, std::wstring applicationName);
	D3DFramework(const D3DFramework& rhs) = delete;
	D3DFramework& operator=(const D3DFramework& rhs) = delete;
	virtual ~D3DFramework();

public:
	virtual bool Initialize() override;
	virtual void OnDestroy() override; 

	virtual void OnResize(int screenWidth, int screenHeight) override;
	virtual void Tick(float deltaTime) override;
	virtual void Render() override;

	virtual void CreateRtvAndDsvDescriptorHeaps();
	
public:
	void ClassifyObjectLayer(std::pair<LayerIter, LayerIter>& layerPair, int i);
	void ClassifyObjectLayer();

public:
	inline static D3DFramework* GetApp() { return static_cast<D3DFramework*>(mApp); }

	inline class Camera* GetCamera() const { return mCamera; }

	inline std::pair<LayerIter, LayerIter>& GetObjectLayer(int layerIndex) { return mLayerPair[layerIndex]; }
	inline std::forward_list<ObjectPtr>& GetAllObjects() { return mAllObjects; }

	inline bool GetIsWireframe() const { return mIsWireframe; }
	inline void SetIsWireframe(bool value) { mIsWireframe = value; }

private:
	void AnimateMaterials(float deltaTime);
	void UpdateObjectCBs(float deltaTime);
	void UpdateMaterialBuffer(float deltaTime);
	void UpdateMainPassCB(float deltaTime);

	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildRenderItems();

	void RenderObjects(ID3D12GraphicsCommandList* cmdList, const LayerIter iterBegin, const LayerIter iterEnd);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	std::array<std::unique_ptr<class FrameResource>, NUM_FRAME_RESOURCES> mFrameResources;
	FrameResource* mCurrentFrameResource = nullptr;
	int mCurrentFrameResourceIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mBillboardLayout;

	// 모든 오브젝트를 담고 있는 포워트 리스트
	// RenderLayer로 정렬하여야 한다.
	std::forward_list<ObjectPtr> mAllObjects;

	// RenderLayer로 분류한 mAllObjects를 각 Layer의 Begin과 End를 담고 있는 Pair 객체
	std::pair<LayerIter, LayerIter> mLayerPair[(int)RenderLayer::Count];

	PassConstants mMainPassCB;
	UINT objCBByteSize = 0;
	UINT passCBByteSize = 0;

	class Camera* mCamera;
	class AssetManager* mAssetManager;

	bool mIsWireframe = false;
};
