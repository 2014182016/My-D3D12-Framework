#pragma once

#include "d3dx12.h"
#include <basetsd.h>

/*
gpu �� �ø� �����͸� ���ε� ���ۿ� �����Ͽ� �޸𸮿� �ø���. 
��� ���ۿ� ��� ������ ũ�Ⱑ 256����Ʈ�� �������� �ϹǷ� 
�����ڿ��� bool���� üũ���־�� �Ѵ�.
*/
template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, const UINT32 elementCount, const bool isConstantBuffer) :
		isConstantBuffer(isConstantBuffer)
	{
		elementByteSize = sizeof(T);

		// ��� ������ ��� elementByteSize�� 256����Ʈ�� ����� �Ͽ��� �Ѵ�.
		// �̴� �ϵ��� m*256 ����Ʈ �����¿��� �����ϴ� n*256����Ʈ ������
		// ��� �ڷḸ �� �� �ֱ� �����̴�.
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT64 SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		if (isConstantBuffer)
			elementByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(T));

		// ���ε� ���۸� �����Ѵ�.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)));

		// ���ε� ������ �޸� �ּҸ� �����Ѵ�.
		ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

		// �ڿ��� �� ����ϱ� ������ ������ ������ �ʿ䰡 ����.
		// �׷���, �ڿ��� GPU�� ����ϴ� �߿��� CPU���� �ڿ��� 
		// �����ϱ� �ʾƾ� �Ѵ�.(���� �ݵ�� ����ȭ ����� ����ؾ� �Ѵ�.) 
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		// �޸𸮸� �����Ѵ�.
		if (uploadBuffer != nullptr)
			uploadBuffer->Unmap(0, nullptr);

		mappedData = nullptr;
	}

	ID3D12Resource* GetResource()const
	{
		return uploadBuffer.Get();
	}

	// �ش� data�� �޸𸮿� �����Ѵ�.
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