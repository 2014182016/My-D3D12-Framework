#include "pch.h"
#include "FrameResource.h"
#include "UploadBuffer.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mCmdListAlloc.GetAddressOf())));

	mPassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
	mMaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
}

FrameResource::~FrameResource() { }