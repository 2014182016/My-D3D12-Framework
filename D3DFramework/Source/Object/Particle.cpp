#include "pch.h"
#include "Particle.h"
#include "Structure.h"
#include "Random.h"
#include "D3DUtil.h"
#include "Material.h"
#include "Global.h"

using namespace std::literals;
using namespace DirectX;
using namespace DirectX::PackedVector;

Particle::Particle(std::string&& name, int maxParticleNum) : Object(std::move(name))
{
	if (maxParticleNum < 0)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "MaxParticleNum is a negative quantity";
#endif
	}
	mMaxParticleNum = maxParticleNum;
}

Particle::~Particle()
{
	mBuffer = nullptr;

	mReadBackCounter = nullptr;
	mBufferCounter = nullptr;

#ifdef BUFFER_COPY
	mReadBackBuffer = nullptr;
#endif
}

void Particle::CreateBuffers(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE bufferCpuSrv = hCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE bufferCpuUav = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE counterCpuUav = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);

	const UINT particleByteSize = mMaxParticleNum * sizeof(ParticleData);
	const UINT counterByteSize = 2 * sizeof(std::uint32_t); 

	D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(particleByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(particleByteSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mBuffer)));

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(counterByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mBufferCounter)));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(counterByteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackCounter)));

#ifdef BUFFER_COPY
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(particleByteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackBuffer)));
#endif

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = mMaxParticleNum;
	srvDesc.Buffer.StructureByteStride = sizeof(ParticleData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	device->CreateShaderResourceView(mBuffer.Get(), &srvDesc, bufferCpuSrv);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = mMaxParticleNum;
	uavDesc.Buffer.StructureByteStride = sizeof(ParticleData);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	device->CreateUnorderedAccessView(mBuffer.Get(), nullptr, &uavDesc, bufferCpuUav);

	D3D12_UNORDERED_ACCESS_VIEW_DESC counterUavDesc = uavDesc;
	counterUavDesc.Buffer.NumElements = 1;
	counterUavDesc.Buffer.StructureByteStride = sizeof(std::uint32_t);
	device->CreateUnorderedAccessView(mBufferCounter.Get(), nullptr, &counterUavDesc, counterCpuUav);
}


void Particle::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	if (GetIsActive())
	{
		if (!mIsInfinite)
		{
			mLifeTime -= deltaTime;
		}

		mSpawnTime -= deltaTime;
	}

#ifdef BUFFER_COPY
	static int frameCount = 0;

	ParticleData* mappedData = nullptr;
	D3D12_RANGE readRange;
	readRange.Begin = 0;
	readRange.End = mMaxParticleNum * sizeof(ParticleData);
	ThrowIfFailed(mReadBackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
	auto name = GetName() + ".txt";
	std::ofstream fout(name.data(), std::ios::app);

	fout << "Particle Num : " << mCurrentParticleNum << std::endl;
	int n = 0;
	for (int i = 0; i < mMaxParticleNum; ++i)
	{
		if (mappedData[i].mIsActive)
		{
			fout << i << " ";
			++n;
			if (n % 20 == 0)
				fout << std::endl;
		}
	}
	fout << std::endl << "Frame : " << frameCount << " end" << std::endl;
	fout << "--------------------------------------------------" << std::endl;

	mReadBackBuffer->Unmap(0, nullptr);

	++frameCount;
#endif
}

void Particle::Render(ID3D12GraphicsCommandList* cmdList, DirectX::BoundingFrustum* frustum) const
{
	if (!GetIsVisible() || mCurrentParticleNum <= 0)
		return;

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	cmdList->DrawInstanced(mMaxParticleNum, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

void Particle::SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = startAddress + mCBIndex * ConstantsSize::particleCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView((UINT)RpParticleCompute::ParticleCB, cbAddress);
}

void Particle::SetBufferSrv(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootShaderResourceView((int)RpParticleGraphics::Buffer, mBuffer->GetGPUVirtualAddress());
}

void Particle::SetBufferUav(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetComputeRootUnorderedAccessView((int)RpParticleCompute::Counter, mBufferCounter->GetGPUVirtualAddress());
	cmdList->SetComputeRootUnorderedAccessView((int)RpParticleCompute::Uav, mBuffer->GetGPUVirtualAddress());
}

void Particle::Emit(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->Dispatch(static_cast<int>(std::ceilf(mEmitNum / NUM_EMIT_THREAD)), 1, 1);

	mSpawnTime = Random::GetRandomFloat(mSpawnTimeRange.first, mSpawnTimeRange.second);
}

void Particle::Update(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->Dispatch(static_cast<int>(std::ceilf(mMaxParticleNum / NUM_UPDATE_THREAD)), 1, 1);
}

void Particle::SetParticleConstants(ParticleConstants& constants)
{
	constants.mStart = mStart;
	constants.mEnd = mEnd;
	constants.mEmitterLocation = GetPosition();
	constants.mMaxParticleNum = (std::uint32_t)mMaxParticleNum;
	constants.mParticleCount = (std::uint32_t)mCurrentParticleNum;
	constants.mEmitNum = (std::uint32_t)mEmitNum;
	constants.mTextureIndex = GetMaterial()->GetDiffuseMapIndex();
}

void Particle::CopyData(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBufferCounter.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	cmdList->CopyResource(mReadBackCounter.Get(), mBufferCounter.Get());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBufferCounter.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

#ifdef BUFFER_COPY
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	cmdList->CopyResource(mReadBackBuffer.Get(), mBuffer.Get());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
#endif

	std::uint32_t* count = nullptr;
	D3D12_RANGE readRange;
	readRange.Begin = 0;
	readRange.End = 1 * sizeof(std::uint32_t);
	ThrowIfFailed(mReadBackCounter->Map(0, &readRange, reinterpret_cast<void**>(&count)));
	mCurrentParticleNum = count[0];
	mReadBackCounter->Unmap(0, nullptr);
}