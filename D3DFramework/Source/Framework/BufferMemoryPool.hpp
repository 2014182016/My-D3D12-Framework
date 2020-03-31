#pragma once

#include "UploadBuffer.h"
#include <memory>

/*
���ε� ������ �޸� ������ �������� ��ȯ�ϱ� ���� Ŭ����.
���ε� ������ ������ 0�� ��� ������ �߻���Ű�Ƿ� 
������ ������ 0�� ��� ���۸� �����ϴ� ���� ���´�.
*/
template<typename T>
class BufferMemoryPool
{
public:
	BufferMemoryPool(ID3D12Device* device, const INT32 bufferCount, const bool isConstantBuffer);
	~BufferMemoryPool();

public:
	// �ش� ���ۿ� count���� �°� ���۸� ���Ҵ��Ѵ�.
	void Resize(const UINT32 bufferCount);

public:
	inline UINT32 GetBufferCount() const { return bufferCount; }
	inline UploadBuffer<T>* GetBuffer() const { return buffer.get(); }

	D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() { if (buffer) return buffer->GetResource()->GetGPUVirtualAddress(); return 0; }

private:
	// ������ �޸� ����
	std::unique_ptr<UploadBuffer<T>> buffer = nullptr;

	// ���۸� ����� ���� ����
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

	// ������ ������ 0�� ��� ���۸� �������� �ʴ´�.
	if(bufferCount != 0)
		buffer = std::make_unique<UploadBuffer<T>>(d3dDevice, bufferCount, isConstantsBuffer);
}

template<typename T>
BufferMemoryPool<T>::~BufferMemoryPool() { }

template<typename T>
void BufferMemoryPool<T>::Resize(const UINT32 bufferCount)
{
	// ���۸� ���Ҵ��ϱ� �� �޸𸮸� �����Ѵ�.
	buffer.reset();

	this->bufferCount = bufferCount;
	// ���۸� ������Ѵ�.
	buffer = std::make_unique<UploadBuffer<T>>(d3dDevice, bufferCount, isConstantsBuffer);
}