#pragma once

#include "D3DStructure.h"
#include "D3DUtil.h"

class ConstantsSize
{
public:
	static inline UINT32 objectCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	static inline UINT32 passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	static inline UINT32 particleCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ParticleConstants));
	static inline UINT32 terrainCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(TerrainConstants));
};

class DescriptorSize
{
public:
	static inline UINT32 rtvDescriptorSize = 0;
	static inline UINT32 dsvDescriptorSize = 0;
	static inline UINT32 cbvSrvUavDescriptorSize = 0;
};

class DescriptorIndex
{
public:
	static inline UINT32 renderTargetHeapIndex = 0;
	static inline UINT32 textureHeapIndex = 0;
	static inline UINT32 shadowMapHeapIndex = 0;
	static inline UINT32 deferredBufferHeapIndex = 0;
	static inline UINT32 depthBufferHeapIndex = 0;
	static inline UINT32 ssaoMapHeapIndex = 0;
	static inline UINT32 ssrMapHeapIndex = 0;
	static inline UINT32 particleHeapIndex = 0;
	static inline UINT32 terrainHeapIndex = 0;
};