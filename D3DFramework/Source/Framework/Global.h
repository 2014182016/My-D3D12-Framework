#pragma once

#include "pch.h"
#include "Structure.h"
#include "D3DUtil.h"

class ConstantsSize
{
public:
	static inline UINT objectCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	static inline UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	static inline UINT particleCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ParticleConstants));
	static inline UINT terrainCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(TerrainConstants));
};

class DescriptorSize
{
public:
	static inline UINT rtvDescriptorSize = 0;
	static inline UINT dsvDescriptorSize = 0;
	static inline UINT cbvSrvUavDescriptorSize = 0;
};

class DescriptorIndex
{
public:
	static inline UINT textureHeapIndex = 0;
	static inline UINT shadowMapHeapIndex = 0;
	static inline UINT deferredBufferHeapIndex = 0;
	static inline UINT ssaoMapHeapIndex = 0;
	static inline UINT particleHeapIndex = 0;
	static inline UINT terrainHeapIndex = 0;
};