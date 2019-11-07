#include "pch.h"
#include "Global.h"

int Material::mCurrentIndex = 0;

Material::Material(std::string name) : mName(name)
{
	mMatCBIndex = mCurrentIndex++;
}

void MeshGeometry::BuildMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
	mIndexCount = (UINT)indices.size();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mVertexBufferCPU));
	CopyMemory(mVertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mIndexBufferCPU));
	CopyMemory(mIndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mVertexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList,
		vertices.data(), vbByteSize, mVertexBufferUploader);

	mIndexBufferGPU = D3DUtil::CreateDefaultBuffer(device, commandList,
		indices.data(), ibByteSize, mIndexBufferUploader);

	mVertexByteStride = sizeof(Vertex);
	mVertexBufferByteSize = vbByteSize;
	mIndexFormat = DXGI_FORMAT_R16_UINT;
	mIndexBufferByteSize = ibByteSize;

	mVertexBufferView.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = mVertexByteStride;
	mVertexBufferView.SizeInBytes = mVertexBufferByteSize;

	mIndexBufferView.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
	mIndexBufferView.Format = mIndexFormat;
	mIndexBufferView.SizeInBytes = mIndexBufferByteSize;
}


void MeshGeometry::Render(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetPrimitiveTopology(mPrimitiveType);
	commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	commandList->IASetIndexBuffer(&mIndexBufferView);

	commandList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
}

void MeshGeometry::DisposeUploaders()
{
	mVertexBufferUploader = nullptr;
	mIndexBufferUploader = nullptr;
}