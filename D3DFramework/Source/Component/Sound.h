#pragma once

#include "pch.h"
#include "Component.h"

class Sound : public Component
{
public:
	Sound(std::string&& name);
	virtual ~Sound();

public:
	void CreateSoundBuffer2D(IDirectSound8* d3dSound, FILE* waveFile, const struct WaveHeaderType& waveFileHeader);
	void CreateSoundBuffer3D(IDirectSound8* d3dSound, FILE* waveFile, const struct WaveHeaderType& waveFileHeader);

	void Play(bool loop);
	void Stop(bool toFirst);
	
public:
	inline DirectX::XMFLOAT3 GetPosition() const { return mPosition; }
	inline bool GetIsPlay() const { return mIsPlay; }
	inline UINT GetVolume() const { return mVolume; }
	inline float GetPan() const { return mPan; }
	inline float GetSpeed() const { return mSpeed; }

	void SetPosition(float x, float y, float z); // Only 3D
	void SetPosition(DirectX::XMFLOAT3 pos); // Only 3D
	void SetBound(float min, float max); // Only 3D

	void SetVolume(UINT volume);
	void SetPan(float pan); // Only 2D
	void SetSpeed(float speed);

protected:
	Microsoft::WRL::ComPtr<IDirectSoundBuffer8> mSoundBuffer;
	Microsoft::WRL::ComPtr<IDirectSound3DBuffer8> mSound3dBuffer;

	DirectX::XMFLOAT3 mPosition;
	bool mIsPlay = false;
	UINT mVolume = 100; // volume�� ������ [0, 100]�̴�.
	float mPan = 0.0f; // -1 �ϼ��� ����, 1 �ϼ��� ������, 0�� �����̴�.
	float mSpeed = 1.0f; // �⺻ �ӵ� 1.0f���� 0�� �������� ������, �� �̻��� ������ �Ѵ�.
};