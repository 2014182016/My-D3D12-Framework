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

	// ��ƼŬ �����͸� ���� ������ ũ�⸦ ����Ѵ�.
	const UINT32 particleByteSize = maxParticleNum * sizeof(ParticleData);
	// 0��° �����ʹ� Ȱ��ȭ�� ��ƼŬ�� ��, 1��° �����ʹ� ���� ������ ��ƼŬ �ε����̴�.
	const UINT32 counterByteSize = 2 * sizeof(UINT32);

	// ���۸� ������ ��, UAV�� ���� ���ҽ��� Flag�� �����ϴ� �Ϳ� �����Ѵ�.
	D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(particleByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(particleByteSize);

	// ��ƼŬ ������ ���۸� �����Ѵ�.
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&particleBuffer)));

	// ���� ī���� ���۸� �����Ѵ�.
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(counterByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&bufferCounter)));

	// ���� ī���͸� ���� ����� ���۸� �����Ѵ�.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(counterByteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&readBackCounter)));

#ifdef BUFFER_COPY
	// ��ƼŬ ������ ���۸� ���� ����� ���۸� �����Ѵ�.
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(particleByteSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackBuffer)));
#endif

	// ��ƼŬ ������ ���ۿ� ���� SRV ������ �並 �����Ѵ�.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxParticleNum;
	srvDesc.Buffer.StructureByteStride = sizeof(ParticleData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	device->CreateShaderResourceView(particleBuffer.Get(), &srvDesc, bufferCpuSrv);

	// ��ƼŬ ������ ���ۿ� ���� SRV ������ �並 �����Ѵ�.
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = maxParticleNum;
	uavDesc.Buffer.StructureByteStride = sizeof(ParticleData);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	device->CreateUnorderedAccessView(particleBuffer.Get(), nullptr, &uavDesc, bufferCpuUav);

	// ���� ī���Ϳ� ���� UAV ������ �並 �����Ѵ�.
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

	// ��� ��ƼŬ �����͸� cpu�� �����Ѵ�.
	ThrowIfFailed(mReadBackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));

	// �����͸� ������ ������ �����Ѵ�.
	auto name = GetName() + ".txt";
	std::ofstream fout(name.data(), std::ios::app);

	// ��ƼŬ �����͸� ����Ѵ�.
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

	// ��ƼŬ�� �������Ѵ�.
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
	// emitNum����ŭ ��ƼŬ�� �����Ѵ�.
	cmdList->Dispatch(static_cast<int>(std::ceilf(emitNum / NUM_EMIT_THREAD)), 1, 1);

	remainingSpawnTime = Random::GetRandomFloat(spawnTimeRange.first, spawnTimeRange.second);
}

void Particle::Update(ID3D12GraphicsCommandList* cmdList)
{
	// ��� ��ƼŬ �����Ϳ� ���Ͽ� ������Ʈ�Ѵ�.
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

	// ���� ī���ͷκ��� ���� Ȱ��ȭ�� ��ƼŬ ������ �о�´�.
	cmdList->CopyResource(readBackCounter.Get(), bufferCounter.Get());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferCounter.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

#ifdef BUFFER_COPY
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particleBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// ������ ��ƼŬ �����͸� �о�´�.
	cmdList->CopyResource(ReadBackBuffer.Get(), particleBuffer.Get());

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(particleBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
#endif

	// Ȱ��ȭ�� ��ƼŬ ������ �����Ѵ�.
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