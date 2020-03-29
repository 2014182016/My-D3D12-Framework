#pragma once

#include <Framework/BufferMemoryPool.hpp>
#include <Framework/D3DStructure.h>
#include <vector>

/*
CPU�� �� �������� ��� ��ϵ��� �����ϴ� �� �ʿ��� �ڿ����� ��ǥ�ϴ� ����ü
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

	// ��Ƽ ������ �������� �ϱ� ���� ��ɾ� ��ü��
	std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> worekrCmdLists;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> workerCmdAllocs;
	// �� �����ӿ��� ����� ��ɾ� ��ü��
	// ��Ƽ �������� ��� FRAME_COUNT��ŭ ������ ��ɾ �����ϱ�
	// �̱� �������� �ϳ��� ��ɾ� ��ü�鸸�� ���ȴ�.
	std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> frameCmdLists;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> frameCmdAllocs;
	// ��Ƽ ������ ���������� ���� ��ɾ� ����Ʈ���� cmdList->ExcuteCommandLists����
	// �ѹ��� �����ϱ� ���Ͽ� �̸� ��ɾ� ����Ʈ �ּҵ��� ��Ƶд�.
	std::vector<ID3D12CommandList*> executableCmdLists;

	// Fence�� ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϴ� ���̴�.
	// �� ���� GPU�� ���� �� ������ �ڿ����� ����ϰ� �ִ��� �����ϴ� �뵵�� ���δ�.
	UINT64 fence = 0;

	// ��� ���۴� �װ��� �����ϴ� ��ɵ��� GPU�� �� ó���� �Ŀ� �����ؾ� �Ѵ�.
	// ���� �����Ӹ��� ��� ���۸� ���� ������ �Ѵ�.
	std::unique_ptr<BufferMemoryPool<PassConstants>> passPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ObjectConstants>> objectPool = nullptr;
	std::unique_ptr<BufferMemoryPool<LightData>> lightBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<MaterialData>> materialBufferPool = nullptr;
	std::unique_ptr<BufferMemoryPool<SsaoConstants>> ssaoPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ObjectConstants>> widgetPool = nullptr;
	std::unique_ptr<BufferMemoryPool<ParticleConstants>> particlePool = nullptr;
	std::unique_ptr<BufferMemoryPool<TerrainConstants>> terrainPool = nullptr;
	std::unique_ptr<BufferMemoryPool<SsrConstants>> ssrPool = nullptr;

	// ������ ���� ���� ���۸� ����Ѵ�.
	// ���� ���� ���۸� �� �����Ӹ��� ���� ���۰� ���� �� �����Ƿ�
	// ������ �ڿ��� �����ϰ�, ���� ���� ���۸� �����ϱ� ���� ���ͷ� �����Ѵ�.
	std::vector<std::unique_ptr<UploadBuffer<WidgetVertex>>> widgetVBs;
};