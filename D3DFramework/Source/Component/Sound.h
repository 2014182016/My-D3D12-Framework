#pragma once

#include "Component.h"

struct WaveHeaderType;
struct IDirectSound8;
struct IDirectSoundBuffer8;
struct IDirectSound3DBuffer;

/*
소리에 관한 정보를 관리하는 클래스
*/
class Sound : public Component
{
public:
	Sound(std::string&& name);
	virtual ~Sound();

public:
	// 2D 전용 사운드에 대한 버퍼를 생성한다.
	void CreateSoundBuffer2D(IDirectSound8* d3dSound, FILE* waveFile, const WaveHeaderType& waveFileHeader);
	// 3D 전용 사운드에 대한 버퍼를 생성한다.
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
	UINT32 volume = 100; // volume의 범위는 [0, 100]이다.
	float pan = 0.0f; // -1 일수록 왼쪽, 1 일수록 오른쪽, 0은 센터이다.
	float speed = 1.0f; // 기본 속도 1.0f에서 0에 가까울수록 느리게, 그 이상은 빠르게 한다.
	bool isPlay = false;
};