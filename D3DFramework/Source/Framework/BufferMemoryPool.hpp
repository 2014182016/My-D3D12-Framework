#pragma once

#include "UploadBuffer.h"
#include <memory>

/*
업로드 버퍼의 메모리 공간을 동적으로 변환하기 위한 클래스.
업로드 버퍼의 공간이 0인 경우 오류를 발생시키므로 
버퍼의 개수가 0일 경우 버퍼를 생성하는 것을 막는다.
*/
template<typename T>
class BufferMemoryPool
{
public:
	BufferMemoryPool(ID3D12Device* device, const INT32 bufferCount, const bool isConstantBuffer);
	~BufferMemoryPool();

public:
	// 해당 버퍼에 count수에 맞게 버퍼를 재할당한다.
	void Resize(const UINT32 bufferCount);

public:
	inline UINT32 GetBufferCount() const { return bufferCount; }
	inline UploadBuffer<T>* GetBuffer() const { return buffer.get(); }

	D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() { if (buffer) return buffer->GetResource()->GetGPUVirtualAddress(); return 0; }

private:
	// 생성된 메모리 버퍼
	std::unique_ptr<UploadBuffer<T>> buffer = nullptr;

	// 버퍼를 만들기 위한 변수
	ID3D12Device* d3dDevice = nullptr;
	UINT32 bufferCount = 0;
	bool isConstantsBuffer = false;
};

template<typename T>
BufferMemoryPool<T>::BufferMemoryPool(ID3D12Device* device, const INT32 bufferCount, const bool IsConstantBuffer)
{
	d3dDevice = device;
	this->isConstantsBuffer = IsConstantBuffer;
	this->bufferCount = bufferCount;

	// 버퍼의 개수가 0일 경우 버퍼를 생성하지 않는다.
	if(bufferCount != 0)
		buffer = std::make_unique<UploadBuffer<T>>(d3dDevice, bufferCount, isConstantsBuffer);
}

template<typename T>
BufferMemoryPool<T>::~BufferMemoryPool() { }

template<typename T>
void BufferMemoryPool<T>::Resize(const UINT32 bufferCount)
{
	// 버퍼를 재할당하기 전 메모리를 해제한다.
	buffer.reset();

	this->bufferCount = bufferCount;
	// 버퍼를 재생성한다.
	buffer = std::make_unique<UploadBuffer<T>>(d3dDevice, bufferCount, isConstantsBuffer);
}