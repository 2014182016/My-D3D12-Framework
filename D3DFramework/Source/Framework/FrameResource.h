#pragma once

#include <Framework/BufferMemoryPool.hpp>
#include <Framework/D3DStructure.h>
#include <vector>

/*
CPU가 한 프레임의 명령 목록들을 구축하는 데 필요한 자원들을 대표하는 구조체
*/
struct FrameResource
{
public:
	FrameResource(ID3D12Device* device, bool isMultiThread,
		const UINT32 passCount, const UINT32 objectCount, const UINT32 lightCount, 
		const UINT32 materialCount, const UINT32 widgetCount, const UINT32 particleCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

public:
	D3D12_GPU_VIRTUAL_ADDRESS GetPassVirtualAddress() const {
		if (passPool->GetBuffer()) return passPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
	D3D12_GPU_VIRTUAL_ADDRESS GetObjectVirtualAddress() const { 
		if (objectPool->GetBuffer()) return objectPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
	D3D12_GPU_VIRTUAL_ADDRESS GetLightVirtualAddress() const { 
		if (lightBufferPool->GetBuffer()) return lightBufferPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
	D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVirtualAddress() const {
		if (materialBufferPool->GetBuffer()) return materialBufferPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
	D3D12_GPU_VIRTUAL_ADDRESS GetSsaoVirtualAddress() const {
		if (ssaoPool->GetBuffer()) return ssaoPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
	D3D12_GPU_VIRTUAL_ADDRESS GetWidgetVirtualAddress() const {
		if (widgetPool->GetBuffer()) return widgetPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
	D3D12_GPU_VIRTUAL_ADDRESS GetParticleVirtualAddress() const {
		if (particlePool->GetBuffer()) return particlePool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
	D3D12_GPU_VIRTUAL_ADDRESS GetTerrainVirtualAddress() const {
		if (terrainPool->GetBuffer()) return terrainPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
	D3D12_GPU_VIRTUAL_ADDRESS GetSsrVirtualAddress() const {
		if (ssrPool->GetBuffer()) return ssrPool->GetBuffer()->GetResource()->GetGPUVirtualAddress(); return NULL; }
 
public:
	static inline UINT32 processorCoreNum = 0;
	static inline UINT32 framePhase = 0;
	static inline bool isInitailize = false;

	// 멀티 스레드 렌더링을 하기 위한 명령어 객체들
	std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> worekrCmdLists;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> workerCmdAllocs;
	// 한 프레임에서 사용할 명령어 객체들
	// 멀티 스레드의 경우 FRAME_COUNT만큼 나누어 명령어를 수행하기
	// 싱글 스레드라면 하나의 명령어 객체들만이 사용된다.
	std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> frameCmdLists;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> frameCmdAllocs;
	// 멀티 스레드 렌더링에서 사용될 명령어 리스트들을 cmdList->ExcuteCommandLists에서
	// 한번에 제출하기 위하여 미리 명령어 리스트 주소들을 담아둔다.
	std::vector<ID3D12CommandList*> executableCmdLists;

	// Fence는 현재 울타리 지점까지의 명령들을 표시하는 값이다.
	// 이 값은 GPU가 아직 이 프레임 자원들을 사용하고 있는지 판정하는 용도로 쓰인다.
	UINT64 fence = 0;

	// 상수 버퍼는 그것을 참조하는 명령들을 GPU가 다 처리한 후에 갱신해야 한다.
	// 따라서 프레임마다 상수 버퍼를 새로 만들어야 한다.
	std::unique_ptr<BufferMemoryPool<PassConstants>> passPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ObjectConstants>> objectPool = nullptr;
	std::unique_ptr<BufferMemoryPool<LightData>> lightBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<MaterialData>> materialBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<SsaoConstants>> ssaoPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ObjectConstants>> widgetPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ParticleConstants>> particlePool = nullptr;
	std::unique_ptr<BufferMemoryPool<TerrainConstants>> terrainPool = nullptr;
	std::unique_ptr<BufferMemoryPool<SsrConstants>> ssrPool = nullptr;

	// 위젯은 동적 정점 버퍼를 사용한다.
	// 동적 정점 버퍼를 각 프레임마다 정점 버퍼가 변할 수 있으므로
	// 프레임 자원에 선언하고, 여러 정점 버퍼를 수용하기 위해 벡터로 관리한다.
	std::vector<std::unique_ptr<UploadBuffer<WidgetVertex>>> widgetVBs;
};