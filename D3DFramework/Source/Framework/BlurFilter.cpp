//***************************************************************************************
// BlurFilter.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include <Framework/BlurFilter.h>
#include <Framework/Enumeration.h>
#include <Framework/D3DInfo.h>

BlurFilter::BlurFilter(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight, const DXGI_FORMAT newFormat)
{
	width = newWidth;
	height = newHeight;
	format = newFormat;

	weights = CalcGaussWeights(2.5f);
	blurRadius = (int)weights.size() / 2;

	BuildResources(device);
}

ID3D12Resource* BlurFilter::Output()
{
	return blurMap1.Get();
}

void BlurFilter::BuildDescriptors(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor)
{
	blur0CpuSrv = hCpuDescriptor;
	blur0CpuUav = hCpuDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	blur1CpuSrv = hCpuDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	blur1CpuUav = hCpuDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);

	blur0GpuSrv = hGpuDescriptor;
	blur0GpuUav = hGpuDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	blur1GpuSrv = hGpuDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	blur1GpuUav = hGpuDescriptor.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->CreateShaderResourceView(blurMap0.Get(), &srvDesc, blur0CpuSrv);
	device->CreateUnorderedAccessView(blurMap0.Get(), nullptr, &uavDesc, blur0CpuUav);

	device->CreateShaderResourceView(blurMap1.Get(), &srvDesc, blur1CpuSrv);
	device->CreateUnorderedAccessView(blurMap1.Get(), nullptr, &uavDesc, blur1CpuUav);
}

void BlurFilter::OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight)
{
	if ((width != newWidth) || (height != newHeight))
	{
		width = newWidth;
		height = newHeight;

		BuildResources(device);
	}
}

void BlurFilter::Execute(ID3D12GraphicsCommandList* cmdList,
	ID3D12PipelineState* horzBlurPSO,
	ID3D12PipelineState* vertBlurPSO,
	ID3D12Resource* input, D3D12_RESOURCE_STATES currState,
	const INT32 blurCount)
{
	cmdList->SetComputeRoot32BitConstants((int)RpBlur::BlurConstants, 1, &blurRadius, 0);
	cmdList->SetComputeRoot32BitConstants((int)RpBlur::BlurConstants, (UINT)weights.size(), weights.data(), 1);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		currState, D3D12_RESOURCE_STATE_COPY_SOURCE));

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(blurMap1.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));

	cmdList->CopyResource(blurMap1.Get(), input);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		D3D12_RESOURCE_STATE_COPY_SOURCE, currState));

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(blurMap1.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	for (int i = 0; i < blurCount; ++i)
	{
		// Horizontal Blur pass.
		cmdList->SetPipelineState(horzBlurPSO);

		cmdList->SetComputeRootDescriptorTable((int)RpBlur::InputSrv, blur1GpuSrv);
		cmdList->SetComputeRootDescriptorTable((int)RpBlur::OutputUav, blur0GpuUav);

		UINT numGroupsX = (UINT)ceilf(width / NUM_THREADS);
		cmdList->Dispatch(numGroupsX, height, 1);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(blurMap1.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(blurMap0.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		// Vertical Blur pass.
		cmdList->SetPipelineState(vertBlurPSO);

		cmdList->SetComputeRootDescriptorTable((int)RpBlur::InputSrv, blur0GpuSrv);
		cmdList->SetComputeRootDescriptorTable((int)RpBlur::OutputUav, blur1GpuUav);

		UINT numGroupsY = (UINT)ceilf(height / NUM_THREADS);
		cmdList->Dispatch(width, numGroupsY, 1);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(blurMap1.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(blurMap0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
}

std::vector<float> BlurFilter::CalcGaussWeights(const float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;
	INT32 blurRadius = (int)ceil(2.0f * sigma);

	assert(blurRadius <= maxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

void BlurFilter::BuildResources(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&blurMap0)));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&blurMap1)));
}