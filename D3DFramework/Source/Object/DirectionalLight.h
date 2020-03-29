#pragma once

#include <Object/Light.h>

/*
태양과 같이 무한히 먼 거리에서 오는 빛
이 라이트로 드리워지는 그림자는 평행하며
전역에 빛이 적용된다.
*/
class DirectionalLight : public Light
{
public:
	DirectionalLight(std::string&& name, ID3D12Device* device);
	virtual ~DirectionalLight();

public:
	virtual void SetLightData(LightData& lightData) override;

};