#pragma once

#include "pch.h"
#include "UploadBuffer.h"

template<typename T>
class BufferMemoryPool
{
public:
	BufferMemoryPool(ID3D12Device* device, int bufferCount, bool isConstantBuffer);
	~BufferMemoryPool();

public:
	// �ش� ���ۿ� count���� �°� ���۸� ���Ҵ��Ѵ�.
	void Resize(UINT bufferCount);

public:
	inline UINT GetBufferCount() const { return mBufferCount; }

	inline UploadBuffer<T>* GetBuffer() const { return mBuffer.get(); }

private:
	// ������ �޸� ����
	std::unique_ptr<UploadBuffer<T>> mBuffer = nullptr;

	// ���۸� ����� ���� ����
	ID3D12Device* mDevice = nullptr;
	UINT mBufferCount = 0;
	bool mIsConstantsBuffer = false;
};

template<typename T>
BufferMemoryPool<T>::BufferMemoryPool(ID3D12Device* device, int bufferCount, bool isConstantBuffer)
{
	mDevice = device;
	mIsConstantsBuffer = isConstantBuffer;

	mBufferCount = bufferCount;

	if(bufferCount != 0)
		mBuffer = std::make_unique<UploadBuffer<T>>(mDevice, mBufferCount, mIsConstantsBuffer);
}

template<typename T>
BufferMemoryPool<T>::~BufferMemoryPool() { }

template<typename T>
void BufferMemoryPool<T>::Resize(UINT bufferCount)
{
	mBuffer.reset();

	mBufferCount = bufferCount;
	mBuffer = std::make_unique<UploadBuffer<T>>(mDevice, mBufferCount, mIsConstantsBuffer);
}