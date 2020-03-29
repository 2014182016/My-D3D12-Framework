#pragma once

#include <Object/Light.h>

/*
한 지점에서 원뿔 모양의 빛을 내뿜는다.
중심 지점에서부터 멀어질수록 빛은 약해진다.
*/
class PointLight : public Light
{
public:
	PointLight(std::string&& name, ID3D12Device* device);
	~PointLight();

public:
	virtual void SetLightData(LightData& lightData) override;

};