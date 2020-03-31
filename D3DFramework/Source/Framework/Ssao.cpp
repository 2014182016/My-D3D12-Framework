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
	// 시그마 값을 계산한다.
	float twoSigma2 = 2.0f * sigma * sigma;
	// 블러 반지름을 측정한다.
	INT32 blurRadius = (int)ceil(2.0f * sigma);

	// 블러 반지름은 최대 반지름을 넘을 수 없다.
	if(blurRadius <= maxBlurRadius)
		blurRadius = maxBlurRadius;

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	// 가중치 값을 계산한다.
	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;
		weights[i + blurRadius] = expf(-x * x / twoSigma2);
		weightSum += weights[i + blurRadius];
	}

	// 가중치 값을 정규화하기 위해 전체 합으로 나눈다.
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
	// Normal Map과 Depth Map은 2개가 연속해서 만들어져있다.
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
		// 앰비언트 맵은 해상도의 절반만을 사용한다.
		renderTargetWidth = newWidth / 2;
		renderTargetHeight = newHeight / 2;

		viewportRect.TopLeftX = 0.0f;
		viewportRect.TopLeftY = 0.0f;
		viewportRect.Width = (float)renderTargetWidth;
		viewportRect.Height = (float)renderTargetHeight;
		viewportRect.MinDepth = 0.0f;
		viewportRect.MaxDepth = 1.0f;

		scissorRect = { 0, 0, (int)renderTargetWidth, (int)renderTargetHeight };

		// 리소스를 생성한다.
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

	// ssao 맵을 지우고, 출력 단계에 바인딩한다.
	cmdList->ClearRenderTargetView(hAmbientMap0CpuRtv, (float*)&clearColor, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &hAmbientMap0CpuRtv, true, nullptr);

	// ssao에 사용할 파이프라인 객체를 바인딩한다.
	cmdList->SetPipelineState(ssaoPso);

	// ssao 맵을 그리기 위한 리소스들을 파이프라인에 바인딩한다.
	cmdList->SetGraphicsRootConstantBufferView((int)RpSsao::SsaoCB, ssaoCBAddress);
	cmdList->SetGraphicsRootConstantBufferView((int)RpSsao::Pass, passCBAddress);
	cmdList->SetGraphicsRoot32BitConstant((int)RpSsao::Constants, 0, 0);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::PositionMap, hPositionMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::BufferMap, hNormalMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::SsaoMap, hRandomVectorMapGpuSrv);

	// ssao 맵을 그린다.
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

	// 블러하기 위해 핑퐁(ping-ponging)을 수행한다.
	// 블러는 수평 블러와 수직 블러로 나누어 지고
	// 수평과 수직에 따라 입력 리소스와 출력 리소스를 설정한다.
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

	// 렌더 타겟은 지우고, 출력 단계에 바인딩한다.
	cmdList->ClearRenderTargetView(outputRtv, (float*)&clearColor, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);

	// 노멀 맵과 사용할 ssao 맵을 파이프라인에 바인딩한다.
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::BufferMap, hNormalMapGpuSrv);
	cmdList->SetGraphicsRootDescriptorTable((int)RpSsao::SsaoMap, inputSrv);

	// ssao 맵을 흐리기한다.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Ssao::BuildResources(ID3D12Device* device)
{
	// 이미 존재하는 리소스를 해제한다.
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

	// 첫 번째 ssao맵을 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&ambientMap0)));

	// 두 번째 ssao맵을 생성한다.
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

	// 랜덤 벡터 맵의 리소스를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&randomVectorMap)));

	// CPU 메모리 데이터를 디폴트 버퍼에 복사하기 위해선
	// 임시적인 업로드 버퍼를 생성해야 한다.

	const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(randomVectorMap.Get(), 0, num2DSubresources);

	// 랜덤 벡터 맵에 복사할 업로드 버퍼를 생성한다.
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
			// 랜덤 벡터 맵에 랜덤한 값을 정의한다.
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
	// 업로드 버퍼를 통해서 데이터를 랜덤 벡터 맵(gpu에 존재)에 복사한다.
	UpdateSubresources(cmdList, randomVectorMap.Get(), randomVectorMapUploadBuffer.Get(),
		0, 0, num2DSubresources, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(randomVectorMap.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Ssao::BuildOffsetVectors()
{
	// 큐브의 각 8개의 꼭짓점과 각 6개의 면의 중심점을 골라
	// SAMPLE_COUNT만큼 분배된 랜덤한 벡터를 생성한다.

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