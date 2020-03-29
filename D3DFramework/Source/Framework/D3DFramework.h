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
	// Renderable 오브젝트를 렌더링한다.
	void RenderObject(ID3D12GraphicsCommandList* cmdList, Renderable* obj,
		D3D12_GPU_VIRTUAL_ADDRESS startAddress, BoundingFrustum* frustum = nullptr) const;
	// Renderabel 오브젝트를 가진 리스트를 이용하여 렌더링한다.
	// 해당 함수를 멀티 스레드 렌더링을 고려하여 스레드 인덱스와 전체 스레드 개수를 인자로 받는다.
	void RenderObjects(ID3D12GraphicsCommandList* cmdList, const std::list<std::shared_ptr<Renderable>>& list,
		D3D12_GPU_VIRTUAL_ADDRESS startAddress, BoundingFrustum* frustum = nullptr, 
		const UINT32 threadIndex = 0, const UINT32  threadNum = 1) const;
	// 화면상에서 실제로 보일 객체들만 그린다.
	void RenderActualObjects(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum = nullptr);
	// 각 스레드마다 지연 렌더링을 수행한다.
	void WorkerThread(const UINT32 threadIndex);

	// 화면 상에 광선을 쏘아 부딪힌 객체를 반환한다. 게임 객체는 충돌 체크를
	// 끌 수 있지만 무조건적인 충돌을 원한다면 isMeshCollision를 true로 한다.
	bool Picking(HitInfo& hitInfo, const INT32 screenX, const INT32 screenY,
		const float distance = 1000.0f, const bool isMeshCollision = false) const;

	// 원하는 오브젝트를 찾아 주소값을 반환한다.
	GameObject* FindGameObject(const std::string name);
	GameObject* FindGameObject(const UINT64 uid);

	// 디버그 하기 위한 함수
	void DrawDebugOctree();
	void DrawDebugCollision();
	void DrawDebugLight();

	Camera* GetCamera() const;

private:
	// 프레임워크를 초기화하고 필요한 객체들을 생성한다.
	void InitFramework();
	void CreateObjects();
	void CreateLights();
	void CreateWidgets(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void CreateParticles();
	void CreateTerrain();
	void CreateFrameResources(ID3D12Device* device);
	void CreateThreads();
	void CreateTerrainStdDevAndNormalMap();

	// 필요한 상수 버퍼들을 업데이트한다.
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

	// 렌더링에 필요한 각 패스들이다.
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

	// 멀티 스레드 렌더링에 필요한 스레드 및 이벤트이다.
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
	
	// 매 프레임마다 카메라 로컬 프러스텀을 월드 좌표계로
	// 변환하여 저장한다. 그 후 프러스텀 컬링할 때, 사용된다.
	BoundingFrustum worldCamFrustum;

private:
	static inline D3DFramework* instance = nullptr;
};
