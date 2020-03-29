#pragma once

#include <Framework/D3DApp.h>
#include <thread>

struct FrameResource;
struct PassConstants;
struct HitInfo;
class Camera;
class Renderable;
class GameObject;
class Light;
class Widget;
class Particle;
class Octree;
class Ssao;
class Ssr;
class BlurFilter;
class D3DDebug;
class Terrain;

class D3DFramework : public D3DApp
{
public:
	D3DFramework(HINSTANCE hInstance, const INT32 screenWidth, const INT32 screenHeight, 
		const std::wstring applicationName, const bool useWinApi = true);
	D3DFramework(const D3DFramework& rhs) = delete;
	D3DFramework& operator=(const D3DFramework& rhs) = delete;
	~D3DFramework();

public:
	virtual bool Initialize() override;
	virtual void OnDestroy() override; 
	virtual void OnResize(const INT32 screenWidth, const INT32 screenHeight) override;
	virtual void Tick(float deltaTime) override;
	virtual void Render() override;
	virtual void CreateDescriptorHeaps(const UINT32 textureNum, const UINT32 shadowMapNum, const UINT32 particleNum) override;

public:
	static D3DFramework* GetInstance();

public:
	// Renderable ������Ʈ�� �������Ѵ�.
	void RenderObject(ID3D12GraphicsCommandList* cmdList, Renderable* obj,
		D3D12_GPU_VIRTUAL_ADDRESS startAddress, BoundingFrustum* frustum = nullptr) const;
	// Renderabel ������Ʈ�� ���� ����Ʈ�� �̿��Ͽ� �������Ѵ�.
	// �ش� �Լ��� ��Ƽ ������ �������� ����Ͽ� ������ �ε����� ��ü ������ ������ ���ڷ� �޴´�.
	void RenderObjects(ID3D12GraphicsCommandList* cmdList, const std::list<std::shared_ptr<Renderable>>& list,
		D3D12_GPU_VIRTUAL_ADDRESS startAddress, BoundingFrustum* frustum = nullptr, 
		const UINT32 threadIndex = 0, const UINT32  threadNum = 1) const;
	// ȭ��󿡼� ������ ���� ��ü�鸸 �׸���.
	void RenderActualObjects(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr);
	// �� �����帶�� ���� �������� �����Ѵ�.
	void WorkerThread(const UINT32 threadIndex);

	// ȭ�� �� ������ ��� �ε��� ��ü�� ��ȯ�Ѵ�. ���� ��ü�� �浹 üũ��
	// �� �� ������ ���������� �浹�� ���Ѵٸ� isMeshCollision�� true�� �Ѵ�.
	bool Picking(HitInfo& hitInfo, const INT32 screenX, const INT32 screenY,
		const float distance = 1000.0f, const bool isMeshCollision = false) const;

	// ���ϴ� ������Ʈ�� ã�� �ּҰ��� ��ȯ�Ѵ�.
	GameObject* FindGameObject(const std::string name);
	GameObject* FindGameObject(const UINT64 uid);

	// ����� �ϱ� ���� �Լ�
	void DrawDebugOctree();
	void DrawDebugCollision();
	void DrawDebugLight();

	Camera* GetCamera() const;

private:
	// �����ӿ�ũ�� �ʱ�ȭ�ϰ� �ʿ��� ��ü���� �����Ѵ�.
	void InitFramework();
	void CreateObjects();
	void CreateLights();
	void CreateWidgets(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void CreateParticles();
	void CreateTerrain();
	void CreateFrameResources(ID3D12Device* device);
	void CreateThreads();
	void CreateTerrainStdDevAndNormalMap();

	// �ʿ��� ��� ���۵��� ������Ʈ�Ѵ�.
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

	// �������� �ʿ��� �� �н����̴�.
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
	void SetDefaultState(ID3D12GraphicsCommandList* cmdList);

private:
	std::array<std::unique_ptr<FrameResource>, NUM_FRAME_RESOURCES> frameResources;
	FrameResource* currentFrameResource = nullptr;
	UINT32 currentFrameResourceIndex = 0;

	// ��Ƽ ������ �������� �ʿ��� ������ �� �̺�Ʈ�̴�.
	std::vector<std::thread> workerThread;
	std::vector<HANDLE> workerBeginFrameEvents;
	std::vector<HANDLE> workerFinishedFrameEvents;

	std::array<std::list<std::shared_ptr<Renderable>>, (int)RenderLayer::Count> renderableObjects;
	std::list<std::shared_ptr<GameObject>> gameObjects;
	std::list<std::shared_ptr<Light>> lights;
	std::list<std::shared_ptr<Widget>> widgets;
	std::list<std::shared_ptr<Particle>> particles;
	std::shared_ptr<Terrain> terrain;

	std::unique_ptr<PassConstants> mainPassCB;
	std::array<std::unique_ptr<PassConstants>, LIGHT_NUM> shadowPassCB;
	std::unique_ptr<Camera> camera;
	std::unique_ptr<Octree> octreeRoot;
	std::unique_ptr<Ssao> ssao;
	std::unique_ptr<Ssr> ssr;
	std::unique_ptr<BlurFilter> blurFilter;
	std::unique_ptr<D3DDebug> d3dDebug;
	
	// �� �����Ӹ��� ī�޶� ���� ���������� ���� ��ǥ���
	// ��ȯ�Ͽ� �����Ѵ�. �� �� �������� �ø��� ��, ���ȴ�.
	BoundingFrustum worldCamFrustum;

private:
	static inline D3DFramework* instance = nullptr;
};
