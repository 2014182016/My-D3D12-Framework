#include "pch.h"
#include "Sound.h"
#include "D3DUtil.h"
#include "Structures.h"

Sound::Sound(std::string&& name) : Component(std::move(name)){ }

Sound::~Sound() { }

void Sound::CreateSoundBuffer2D(IDirectSound8* d3dSound, FILE* waveFile, const WaveHeaderType& waveFileHeader)
{
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = waveFileHeader.audioFormat;
	waveFormat.nSamplesPerSec = waveFileHeader.sampleRate;
	waveFormat.wBitsPerSample = waveFileHeader.bitsPerSample;
	waveFormat.nChannels = waveFileHeader.numChannels;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	DSBUFFERDESC bufferDesc;
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE | DSBCAPS_STATIC;
	bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &waveFormat;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	IDirectSoundBuffer* tempBuffer = nullptr;
	ThrowIfFailed(d3dSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL));
	tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)mSoundBuffer.GetAddressOf());
	tempBuffer->Release();

	unsigned char* waveData = new unsigned char[waveFileHeader.dataSize];
	fseek(waveFile, sizeof(WaveHeaderType), SEEK_SET);
	fread(waveData, 1, waveFileHeader.dataSize, waveFile);
	fclose(waveFile);

	unsigned char* bufferPtr = nullptr;
	unsigned long bufferSize = 0;
	ThrowIfFailed(mSoundBuffer->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0));
	memcpy(bufferPtr, waveData, waveFileHeader.dataSize);
	ThrowIfFailed(mSoundBuffer->Unlock((void*)bufferPtr, bufferSize, NULL, 0));

	mSoundBuffer->SetFormat(&waveFormat);

	delete[] waveData;
	waveData = 0;
}

void Sound::CreateSoundBuffer3D(IDirectSound8* d3dSound, FILE* waveFile, const WaveHeaderType& waveFileHeader)
{
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = waveFileHeader.audioFormat;
	waveFormat.nSamplesPerSec = waveFileHeader.sampleRate;
	waveFormat.wBitsPerSample = waveFileHeader.bitsPerSample;
	waveFormat.nChannels = 1;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	DSBUFFERDESC bufferDesc;
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE | DSBCAPS_STATIC;
	bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &waveFormat;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	IDirectSoundBuffer* tempBuffer = nullptr;
	ThrowIfFailed(d3dSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL));
	tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)mSoundBuffer.GetAddressOf());
	tempBuffer->Release();

	unsigned char* waveData = new unsigned char[waveFileHeader.dataSize];
	fseek(waveFile, sizeof(WaveHeaderType), SEEK_SET);
	fread(waveData, 1, waveFileHeader.dataSize, waveFile);
	fclose(waveFile);

	unsigned char* bufferPtr = nullptr;
	unsigned long bufferSize = 0;
	ThrowIfFailed(mSoundBuffer->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0));
	memcpy(bufferPtr, waveData, waveFileHeader.dataSize);
	ThrowIfFailed(mSoundBuffer->Unlock((void*)bufferPtr, bufferSize, NULL, 0));

	mSoundBuffer->SetFormat(&waveFormat);
	ThrowIfFailed(mSoundBuffer->QueryInterface(IID_IDirectSound3DBuffer8, (void**)mSound3dBuffer.GetAddressOf()));

	delete[] waveData;
	waveData = 0;
}

void Sound::Play(bool loop)
{
	mSoundBuffer->Play(0, 0, (loop ? 1 : 0));
	mIsPlay = true;
}

void Sound::Stop(bool toFirst)
{
	mSoundBuffer->Stop();
	mIsPlay = false;

	if (toFirst)
	{
		mSoundBuffer->SetCurrentPosition(0L);
	}
}

void Sound::SetPosition(float x, float y, float z)
{
	if (mSound3dBuffer)
		mSound3dBuffer->SetPosition(x, y, z, DS3D_IMMEDIATE);
}

void Sound::SetPosition(DirectX::XMFLOAT3 pos)
{
	SetPosition(pos.x, pos.y, pos.z);
}

void Sound::SetBound(float min, float max)
{
	mSound3dBuffer->SetMinDistance(min, DS3D_IMMEDIATE);
	mSound3dBuffer->SetMaxDistance(max, DS3D_IMMEDIATE);
}

void Sound::SetVolume(UINT volume)
{
	mVolume = volume;
	// [0, 100]인 볼륨을 DirectSound에서 사용할 수 있도록 [-10000, 0]으로 변환한다.
	LONG dsbVolume = (LONG)(((float(volume) / 100.0f) - 1.0f) * DSBVOLUME_MIN);
	mSoundBuffer->SetVolume(dsbVolume);
}

void Sound::SetPan(float pan)
{
	mPan = pan;
	// [-1, 1]인 Pan을 DirectSound에서 사용할 수 있도록 [-10000, 10000]으로 변환한다.
	LONG dsbpPan = (LONG)(mPan * 10000.0f);
	mSoundBuffer->SetPan(dsbpPan);
}

void Sound::SetSpeed(float speed)
{
	mSpeed = speed;

	DWORD dsbFrequency;
	mSoundBuffer->GetFrequency(&dsbFrequency);
	dsbFrequency = (DWORD)(mSpeed * dsbFrequency);
	dsbFrequency = std::clamp<DWORD>(dsbFrequency, DSBFREQUENCY_MIN, DSBFREQUENCY_MAX);

	mSoundBuffer->SetFrequency(dsbFrequency);
}