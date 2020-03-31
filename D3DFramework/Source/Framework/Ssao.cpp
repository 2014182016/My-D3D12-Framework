//***************************************************************************************
// Ssao.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "../PrecompiledHeader/pch.h"
#include "Ssao.h"
#include "D3DInfo.h"
#include "Random.h"
#include "Enumeration.h"

Ssao::Ssao(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const UINT32 width, const UINT32 height)
{
	OnResize(device, width, height);

	BuildOffsetVectors();
	BuildRandomVectorTexture(device, cmdList);
}

std::vector<float> Ssao::CalcGaussWeights(const float sigma) const
{
	// �ñ׸� ���� ����Ѵ�.
	float twoSigma2 = 2.0f * sigma * sigma;
	// �� �������� �����Ѵ�.
	INT32 blurRadius = (int)ceil(2.0f * sigma);

	// �� �������� �ִ� �������� ���� �� ����.
	if(blurRadius <= maxBlurRadius)
		blurRadius = maxBlurRadius;

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	// ����ġ ���� ����Ѵ�.
	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;
		weights[i + blurRadius] = expf(-x * x / twoSigma2);
		weightSum += weights[i + blurRadius];
	}

	// ����ġ ���� ����ȭ�ϱ� ���� ��ü ������ ������.
	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

void Ssao::BuildDescriptors(ID3D12Device* device, 
	CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalMapGpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hPositionMapGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
	// Normal Map�� Depth Map�� 2���� �����ؼ� ��������ִ�.
	this->hNormalMapGpuSrv = hNormalMapGpuSrv;
	this->hPositionMapGpuSrv = hPositionMapGpuSrv;

	hAmbientMap0CpuSrv = hCpuSrv;
	hAmbientMap1CpuSrv = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	hRandomVectorMapCpuSrv = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);

	hAmbientMap0GpuSrv = hGpuSrv;
	hAmbientMap1GpuSrv = hGpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	hRandomVectorMapGpuSrv = hGpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);

	hAmbientMap0CpuRtv = hCpuRtv;
	hAmbientMap1CpuRtv = hCpuRtv.Offset(1, DescriptorSize::rtvDescriptorSize);

	RebuildDescriptors(device);
}

void Ssao::RebuildDescriptors(ID3D12Device* device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(randomVectorMap.Get(), &srvDesc, hRandomVectorMapCpuSrv);

	srvDesc.Format = ambientMapFormat;
	device->CreateShaderResourceView(ambientMap0.Get(), &srvDesc, hAmbientMap0CpuSrv);
	device->CreateShaderResourceView(ambientMap1.Get(), &srvDesc, hAmbientMap1CpuSrv);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = ambientMapFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	device->CreateRenderTargetView(ambientMap0.Get(), &rtvDesc, hAmbientMap0CpuRtv);
	device->CreateRenderTargetView(ambientMap1.Get(), &rtvDesc, hAmbientMap1CpuRtv);
}

void Ssao::OnResize(ID3D12Device* device, const UINT32 newWidth, const UINT32 newHeight)
{
	if (renderTargetWidth != newWidth || renderTargetHeight != newHeight)
	{
		// �ں��Ʈ ���� �ػ��� ���ݸ��� ����Ѵ�.
		renderTargetWidth = newWidth / 2;
		renderTargetHeight = newHeight / 2;

		viewportRect.TopLeftX = 0.0f;
		viewportRect.TopLeftY = 0.0f;
		viewportRect.Width = (float)renderTargetWidth;
		viewportRect.Height = (float)renderTargetHeight;
		viewportRect.MinDepth = 0.0f;
		viewportRect.MaxDepth = 1.0f;

		scissorRect = { 0, 0, (int)renderTargetWidth, (int)renderTargetHeight };

		// ���ҽ��� �����Ѵ�.
		BuildResources(device);
	}
}

void Ssao::ComputeSsao(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* ssaoPso, 
	D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddress, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress)
{
	cmdList->RSSetViewports(1, &viewportRect);
	cmdList->RSSetScissorRects(1, &scissorRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ambientMap0.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ssao ���� �����, ��� �ܰ迡 ���ε��Ѵ�.
	cmdList->ClearRenderTargetView(hAmbientMap0CpuRtv, (float*)&clearColor, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &hAmbientMap0CpuRtv, true, nullptr);

	// ssao�� ����� ���������� ��ü�� ���ε��Ѵ�.
	cmdList->SetPipelineState(ssaoPso);

	// ssao ���� �׸��� ���� ���ҽ����� ���������ο� ���ε��Ѵ�.
	cmdList->SetGraphicsRootConstantBufferView((int)RpSsao::SsaoCB, ssaoCBAddress);
	cmdList->SetGraphicsRootConstantBufferView((int)RpSsao::Pass, passCBAddress);
	cmdList->SetGraphicsRoot32BitConstant((int)RpSsao::Constants, 0, 0);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::PositionMap, hPositionMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::BufferMap, hNormalMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::SsaoMap, hRandomVectorMapGpuSrv);

	// ssao ���� �׸���.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ambientMap0.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* blurPso, const INT32 blurCount)
{
	cmdList->SetPipelineState(blurPso);

	for (int i = 0; i < blurCount; ++i)
	{
		BlurAmbientMap(cmdList, true);
		BlurAmbientMap(cmdList, false);
	}
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur)
{
	ID3D12Resource* output = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE inputSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;

	// ���ϱ� ���� ����(ping-ponging)�� �����Ѵ�.
	// ���� ���� ���� ���� ���� ������ ����
	// ����� ������ ���� �Է� ���ҽ��� ��� ���ҽ��� �����Ѵ�.
	if (horzBlur == true)
	{
		output = ambientMap1.Get();
		inputSrv = hAmbientMap0GpuSrv;
		outputRtv = hAmbientMap1CpuRtv;
		cmdList->SetGraphicsRoot32BitConstant((int)RpSsao::Constants, 1, 0);
	}
	else
	{
		output = ambientMap0.Get();
		inputSrv = hAmbientMap1GpuSrv;
		outputRtv = hAmbientMap0CpuRtv;
		cmdList->SetGraphicsRoot32BitConstant((int)RpSsao::Constants, 0, 0);
	}

	cmdList->RSSetViewports(1, &viewportRect);
	cmdList->RSSetScissorRects(1, &scissorRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ���� Ÿ���� �����, ��� �ܰ迡 ���ε��Ѵ�.
	cmdList->ClearRenderTargetView(outputRtv, (float*)&clearColor, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);

	// ��� �ʰ� ����� ssao ���� ���������ο� ���ε��Ѵ�.
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::BufferMap, hNormalMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::SsaoMap, inputSrv);

	// ssao ���� �帮���Ѵ�.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Ssao::BuildResources(ID3D12Device* device)
{
	// �̹� �����ϴ� ���ҽ��� �����Ѵ�.
	ambientMap0 = nullptr;
	ambientMap1 = nullptr;

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = renderTargetWidth;
	texDesc.Height = renderTargetHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = Ssao::ambientMapFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	CD3DX12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(ambientMapFormat, (float*)&clearColor);

	// ù ��° ssao���� �����Ѵ�.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&ambientMap0)));

	// �� ��° ssao���� �����Ѵ�.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&ambientMap1)));
}

void Ssao::BuildRandomVectorTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = RANDOM_VECTORMAP_SIZE;
	texDesc.Height = RANDOM_VECTORMAP_SIZE;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// ���� ���� ���� ���ҽ��� �����Ѵ�.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&randomVectorMap)));

	// CPU �޸� �����͸� ����Ʈ ���ۿ� �����ϱ� ���ؼ�
	// �ӽ����� ���ε� ���۸� �����ؾ� �Ѵ�.

	const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(randomVectorMap.Get(), 0, num2DSubresources);

	// ���� ���� �ʿ� ������ ���ε� ���۸� �����Ѵ�.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(randomVectorMapUploadBuffer.GetAddressOf())));

	PackedVector::XMCOLOR initData[RANDOM_VECTORMAP_SIZE * RANDOM_VECTORMAP_SIZE];
	for (int i = 0; i < RANDOM_VECTORMAP_SIZE; ++i)
	{
		for (int j = 0; j < RANDOM_VECTORMAP_SIZE; ++j)
		{
			// ���� ���� �ʿ� ������ ���� �����Ѵ�.
			XMFLOAT3 v(Random::GetRandomFloat(0.0f, 1.0f), Random::GetRandomFloat(0.0f, 1.0f), Random::GetRandomFloat(0.0f, 1.0f));
			initData[i * 256 + j] = PackedVector::XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = RANDOM_VECTORMAP_SIZE * sizeof(PackedVector::XMCOLOR);
	subResourceData.SlicePitch = subResourceData.RowPitch * RANDOM_VECTORMAP_SIZE;

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(randomVectorMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	// ���ε� ���۸� ���ؼ� �����͸� ���� ���� ��(gpu�� ����)�� �����Ѵ�.
	UpdateSubresources(cmdList, randomVectorMap.Get(), randomVectorMapUploadBuffer.Get(),
		0, 0, num2DSubresources, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(randomVectorMap.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Ssao::BuildOffsetVectors()
{
	// ť���� �� 8���� �������� �� 6���� ���� �߽����� ���
	// SAMPLE_COUNT��ŭ �й�� ������ ���͸� �����Ѵ�.

	// 8 cube corners
	offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

	for (int i = 0; i < SAMPLE_COUNT; ++i)
	{
		float length = Random::GetRandomFloat(0.25f, 1.0f);
		XMVECTOR offeset = length * XMVector4Normalize(XMLoadFloat4(&offsets[i]));
		XMStoreFloat4(&offsets[i], offeset);
	}
}

void Ssao::GetOffsetVectors(XMFLOAT4* offsets)
{
	for (int i = 0; i < SAMPLE_COUNT; ++i)
	{
		offsets[i] = this->offsets[i];
	}
}

UINT32 Ssao::GetWidth() const
{
	return renderTargetWidth;
}

UINT32 Ssao::GetHeight() const
{
	return renderTargetHeight;
}