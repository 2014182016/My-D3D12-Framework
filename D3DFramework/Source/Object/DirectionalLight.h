#pragma once

#include <Object/Light.h>

/*
�¾�� ���� ������ �� �Ÿ����� ���� ��
�� ����Ʈ�� �帮������ �׸��ڴ� �����ϸ�
������ ���� ����ȴ�.
*/
class DirectionalLight : public Light
{
public:
	DirectionalLight(std::string&& name, ID3D12Device* device);
	virtual ~DirectionalLight();

public:
	virtual void SetLightData(LightData& lightData) override;

};