#pragma once

#include "d3dx12.h"
#include <basetsd.h>

/*
gpu 상에 올릴 데이터를 업로드 버퍼에 복사하여 메모리에 올린다. 
상수 버퍼에 경우 버퍼의 크기가 256바이트의 배수에어야 하므로 
생성자에서 bool값을 체크해주어야 한다.
*/
template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, const UINT32 elementCount, const bool isConstantBuffer) :
		isConstantBuffer(isConstantBuffer)
	{
		elementByteSize = sizeof(T);

		// 상수 버퍼일 경우 elementByteSize는 256바이트의 배수로 하여야 한다.
		// 이는 하드웨어가 m*256 바이트 오프셋에서 시작하는 n*256바이트 길이의
		// 상수 자료만 볼 수 있기 때문이다.
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT64 SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		if (isConstantBuffer)
			elementByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(T));

		// 업로드 버퍼를 생성한다.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)));

		// 업로드 버퍼의 메모리 주소를 매핑한다.
		ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

		// 자원을 다 사용하기 전에는 대응을 해제할 필요가 없다.
		// 그러나, 자원을 GPU가 사용하는 중에는 CPU에서 자원을 
		// 갱신하기 않아야 한다.(따라서 반드시 동기화 기법을 사용해야 한다.) 
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		// 메모리를 해제한다.
		if (uploadBuffer != nullptr)
			uploadBuffer->Unmap(0, nullptr);

		mappedData = nullptr;
	}

	ID3D12Resource* GetResource()const
	{
		return uploadBuffer.Get();
	}

	// 해당 data를 메모리에 복사한다.
	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mappedData[elementIndex*elementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
	BYTE* mappedData = nullptr;

	UINT32 elementByteSize = 0;
	bool isConstantBuffer = false;
};