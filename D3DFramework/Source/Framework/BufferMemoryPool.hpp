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
	// 해당 버퍼에 count수에 맞게 버퍼를 재할당한다.
	void Resize(UINT bufferCount);

public:
	inline UINT GetBufferCount() const { return mBufferCount; }

	inline UploadBuffer<T>* GetBuffer() const { return mBuffer.get(); }

private:
	// 생성된 메모리 버퍼
	std::unique_ptr<UploadBuffer<T>> mBuffer = nullptr;

	// 버퍼를 만들기 위한 변수
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