#include <Object/Particle.h>
#include <Framework/D3DInfo.h>
#include <Framework/Random.h>
#include <Component/Material.h>
#include <iostream>

Particle::Particle(std::string&& name, int maxParticleNum) : Object(std::move(name))
{
	if (maxParticleNum < 0)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << "MaxParticleNum is a negative quantity";
#endif
	}
	maxParticleNum = maxParticleNum;
}

Particle::~Particle()
{
	particleBuffer = nullptr;
	readBackCounter = nullptr;
	bufferCounter = nullptr;
#ifdef BUFFER_COPY
	mReadBackBuffer = nullptr;
#endif
}

void Particle::CreateBuffers(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE bufferCpuSrv = hCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE bufferCpuUav = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE counterCpuUav = hCpuSrv.Offset(1, DescriptorSize::cbvSrvUavDescriptorSize);

	// 파티클 데이터를 담을 버퍼의 크기를 계산한다.
	const UINT32 particleByteSize = maxParticleNum * sizeof(ParticleData);
	// 0번째 데이터는 활성화된 파티클의 수, 1번째 데이터는 현재 생성할 파티클 인덱스이다.
	const UINT32 counterByteSize = 2 * sizeof(UINT32);

	// 버퍼를 생성할 때, UAV로 묶을 리소스는 Flag를 설정하는 것에 주의한다.
	D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(particleByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(particleByteSize);

	// 파티클 데이터 버퍼를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&particleBuffer)));

	// 버퍼 카운터 버퍼를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(counterByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&bufferCounter)));

	// 버퍼 카운터를 읽을 리드백 버퍼를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(counterByteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&readBackCounter)));

#ifdef BUFFER_COPY
	// 파티클 데이터 버퍼를 읽을 리드백 버퍼를 생성한다.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(particleByteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackBuffer)));
#endif

	// 파티클 데이터 버퍼에 대한 SRV 서술자 뷰를 생성한다.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxParticleNum;
	srvDesc.Buffer.StructureByteStride = sizeof(ParticleData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	device->CreateShaderResourceView(particleBuffer.Get(), &srvDesc, bufferCpuSrv);

	// 파티클 데이터 버퍼에 대한 SRV 서술자 뷰를 생성한다.
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = maxParticleNum;
	uavDesc.Buffer.StructureByteStride = sizeof(ParticleData);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	device->CreateUnorderedAccessView(particleBuffer.Get(), nullptr, &uavDesc, bufferCpuUav);

	// 버퍼 카운터에 대한 UAV 서술자 뷰를 생성한다.
	D3D12_UNORDERED_ACCESS_VIEW_DESC counterUavDesc = uavDesc;
	counterUavDesc.Buffer.NumElements = 1;
	counterUavDesc.Buffer.StructureByteStride = sizeof(std::uint32_t);
	device->CreateUnorderedAccessView(bufferCounter.Get(), nullptr, &counterUavDesc, counterCpuUav);
}


void Particle::Tick(float deltaTime)
{
	__super::Tick(deltaTime);

	if (isActive)
	{
		if (!isInfinite)
		{
			lifeTime -= deltaTime;
		}

		remainingSpawnTime -= deltaTime;
	}

#ifdef BUFFER_COPY
	static int frameCount = 0;

	ParticleData* mappedData = nullptr;
	D3D12_RANGE readRange;
	readRange.Begin = 0;
	readRange.End = maxParticleNum * sizeof(ParticleData);

	// 모든 파티클 데이터를 cpu로 복사한다.
	ThrowIfFailed(mReadBackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));

	// 데이터를 복사할 파일을 선언한다.
	auto name = GetName() + ".txt";
	std::ofstream fout(name.data(), std::ios::app);

	// 파티클 데이터를 출력한다.
	fout << "Particle Num : " << currentParticleNum << std::endl;
	int n = 0;
	for (int i = 0; i < maxParticleNum; ++i)
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

void Particle::Render(ID3D12GraphicsCommandList* cmdList, BoundingFrustum* frustum) const
{
	if (!isVisible || currentParticleNum <= 0)
		return;

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particleBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

	// 파티클을 렌더링한다.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	cmdList->DrawInstanced(maxParticleNum, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particleBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

void Particle::SetConstantBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS startAddress) const
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = startAddress + cbIndex * ConstantsSize::particleCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView((UINT)RpParticleCompute::ParticleCB, cbAddress);
}

void Particle::SetBufferSrv(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootShaderResourceView((UINT)RpParticleGraphics::Buffer, particleBuffer->GetGPUVirtualAddress());
}

void Particle::SetBufferUav(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetComputeRootUnorderedAccessView((UINT)RpParticleCompute::Counter, bufferCounter->GetGPUVirtualAddress());
	cmdList->SetComputeRootUnorderedAccessView((UINT)RpParticleCompute::Uav, particleBuffer->GetGPUVirtualAddress());
}

void Particle::Emit(ID3D12GraphicsCommandList* cmdList)
{
	// emitNum수만큼 파티클을 생성한다.
	cmdList->Dispatch(static_cast<int>(std::ceilf(emitNum / NUM_EMIT_THREAD)), 1, 1);

	remainingSpawnTime = Random::GetRandomFloat(spawnTimeRange.first, spawnTimeRange.second);
}

void Particle::Update(ID3D12GraphicsCommandList* cmdList)
{
	// 모든 파티클 데이터에 대하여 업데이트한다.
	cmdList->Dispatch(static_cast<int>(std::ceilf(maxParticleNum / NUM_UPDATE_THREAD)), 1, 1);
}

void Particle::SetParticleConstants(ParticleConstants& constants)
{
	constants.start = start;
	constants.end = end;
	constants.emitterLocation = position;
	constants.enabledGravity = enabledGravity;
	constants.maxParticleNum = maxParticleNum;
	constants.particleCount = currentParticleNum;
	constants.emitNum = emitNum;
	constants.textureIndex = material->diffuseMapIndex;
}

void Particle::CopyData(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferCounter.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// 버퍼 카운터로부터 현재 활성화된 파티클 개수를 읽어온다.
	cmdList->CopyResource(readBackCounter.Get(), bufferCounter.Get());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferCounter.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

#ifdef BUFFER_COPY
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particleBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// 복사할 파티클 데이터를 읽어온다.
	cmdList->CopyResource(ReadBackBuffer.Get(), particleBuffer.Get());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particleBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
#endif

	// 활성화된 파티클 개수를 복사한다.
	UINT32* count = nullptr;
	D3D12_RANGE readRange;
	readRange.Begin = 0;
	readRange.End = 1 * sizeof(UINT32);
	ThrowIfFailed(readBackCounter->Map(0, &readRange, reinterpret_cast<void**>(&count)));
	currentParticleNum = count[0];
	readBackCounter->Unmap(0, nullptr);
}

void Particle::SetMaterial(Material* material)
{
	this->material = material;
}

Material* Particle::GetMaterial() const
{
	return material;
}

bool Particle::IsSpawnable() const
{
	if (remainingSpawnTime < 0.0f && currentParticleNum < maxParticleNum)
		return true;
	return false;
}