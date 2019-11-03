#pragma once

#include "D3DApp.h"

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

class Camera;
class FrameResource;

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

private:
	virtual void OnResize(int screenWidth, int screenHeight) override;
	virtual void Tick(float deltaTime) override;
	virtual void Render() override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	virtual void OnKeyDown(unsigned int input) override;
	virtual void OnKeyUp(unsigned int input) override;

	// 매 프레임마다 Input관련 처리를 하기 위한 함수
	virtual void OnProcessInput(float deltaTime);

private:
	virtual void CreateRtvAndDsvDescriptorHeaps();

	void AnimateMaterials(float deltaTime);
	void UpdateObjectCBs(float deltaTime);
	void UpdateMaterialBuffer(float deltaTime);
	void UpdateMainPassCB(float deltaTime);

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildBBGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	std::array<std::unique_ptr<FrameResource>, NUM_FRAME_RESOURCES> mFrameResources;
	FrameResource* mCurrentFrameResource = nullptr;
	int mCurrentFrameResourceIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavDescriptorHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mBillboardLayout;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	UINT objCBByteSize = 0;
	UINT passCBByteSize = 0;

	PassConstants mMainPassCB; 

	Camera* mCamera;
	float mCameraWalkSpeed = 50.0f;
	float mCameraRotateSpeed = 0.25f;

	POINT mLastMousePos;

	UINT mTextureNum;

	bool mIsWireframe = false;
};
