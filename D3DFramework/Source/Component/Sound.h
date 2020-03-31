#pragma once

#include "Component.h"

struct WaveHeaderType;
struct IDirectSound8;
struct IDirectSoundBuffer8;
struct IDirectSound3DBuffer;

/*
�Ҹ��� ���� ������ �����ϴ� Ŭ����
*/
class Sound : public Component
{
public:
	Sound(std::string&& name);
	virtual ~Sound();

public:
	// 2D ���� ���忡 ���� ���۸� �����Ѵ�.
	void CreateSoundBuffer2D(IDirectSound8* d3dSound, FILE* waveFile, const WaveHeaderType& waveFileHeader);
	// 3D ���� ���忡 ���� ���۸� �����Ѵ�.
	void CreateSoundBuffer3D(IDirectSound8* d3dSound, FILE* waveFile, const WaveHeaderType& waveFileHeader);

	void Play(const bool loop);
	void Stop(const bool toFirst);
	
	void SetPosition(const float x, const float y, const float z); // Only 3D
	void SetPosition(const XMFLOAT3& pos); // Only 3D
	void SetBound(const float min, const float max); // Only 3D

	void SetVolume(const UINT32 volume);
	void SetPan(const float pan); // Only 2D
	void SetSpeed(const float speed);

protected:
	Microsoft::WRL::ComPtr<IDirectSoundBuffer8> soundBuffer;
	Microsoft::WRL::ComPtr<IDirectSound3DBuffer8> sound3dBuffer;

	XMFLOAT3 position;
	UINT32 volume = 100; // volume�� ������ [0, 100]�̴�.
	float pan = 0.0f; // -1 �ϼ��� ����, 1 �ϼ��� ������, 0�� �����̴�.
	float speed = 1.0f; // �⺻ �ӵ� 1.0f���� 0�� �������� ������, �� �̻��� ������ �Ѵ�.
	bool isPlay = false;
};