#include "../PrecompiledHeader/pch.h"
#include "Sound.h"
#include "../Framework/AssetLoader.h"

Sound::Sound(std::string&& name) : Component(std::move(name)){ }

Sound::~Sound() { }

void Sound::CreateSoundBuffer2D(IDirectSound8* d3dSound, FILE* waveFile, const WaveHeaderType& waveFileHeader)
{
	// 웨이브 포맷을 정의한다.
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = waveFileHeader.audioFormat;
	waveFormat.nSamplesPerSec = waveFileHeader.sampleRate;
	waveFormat.wBitsPerSample = waveFileHeader.bitsPerSample;
	waveFormat.nChannels = waveFileHeader.numChannels;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// 버퍼 서술자를 정의한다.
	DSBUFFERDESC bufferDesc;
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE | DSBCAPS_STATIC;
	bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &waveFormat;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// 사운드 버퍼를 생성한다.
	IDirectSoundBuffer* tempBuffer = nullptr;
	ThrowIfFailed(d3dSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL));
	tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)soundBuffer.GetAddressOf());
	tempBuffer->Release();

	// 로드한 사운드 데이터를 복사한다.
	unsigned char* waveData = new unsigned char[waveFileHeader.dataSize];
	fseek(waveFile, sizeof(WaveHeaderType), SEEK_SET);
	fread(waveData, 1, waveFileHeader.dataSize, waveFile);
	fclose(waveFile);

	// gpu로 사운드 데이터를 복사한다.
	unsigned char* bufferPtr = nullptr;
	unsigned long bufferSize = 0;
	ThrowIfFailed(soundBuffer->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0));
	memcpy(bufferPtr, waveData, waveFileHeader.dataSize);
	ThrowIfFailed(soundBuffer->Unlock((void*)bufferPtr, bufferSize, NULL, 0));

	soundBuffer->SetFormat(&waveFormat);

	// 복사한 데이터는 더이상 필요 없으므로 삭제한다.
	delete[] waveData;
	waveData = 0;
}

void Sound::CreateSoundBuffer3D(IDirectSound8* d3dSound, FILE* waveFile, const WaveHeaderType& waveFileHeader)
{
	// 웨이브 포맷을 정의한다.
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = waveFileHeader.audioFormat;
	waveFormat.nSamplesPerSec = waveFileHeader.sampleRate;
	waveFormat.wBitsPerSample = waveFileHeader.bitsPerSample;
	waveFormat.nChannels = 1;
	waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	// 버퍼 서술자를 정의한다.
	DSBUFFERDESC bufferDesc;
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE | DSBCAPS_STATIC;
	bufferDesc.dwBufferBytes = waveFileHeader.dataSize;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = &waveFormat;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// 사운드 버퍼를 생성한다.
	IDirectSoundBuffer* tempBuffer = nullptr;
	ThrowIfFailed(d3dSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL));
	tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)soundBuffer.GetAddressOf());
	tempBuffer->Release();

	// 로드한 사운드 데이터를 복사한다.
	unsigned char* waveData = new unsigned char[waveFileHeader.dataSize];
	fseek(waveFile, sizeof(WaveHeaderType), SEEK_SET);
	fread(waveData, 1, waveFileHeader.dataSize, waveFile);
	fclose(waveFile);

	// gpu로 사운드 데이터를 복사한다.
	unsigned char* bufferPtr = nullptr;
	unsigned long bufferSize = 0;
	ThrowIfFailed(soundBuffer->Lock(0, waveFileHeader.dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0));
	memcpy(bufferPtr, waveData, waveFileHeader.dataSize);
	ThrowIfFailed(soundBuffer->Unlock((void*)bufferPtr, bufferSize, NULL, 0));

	// 3D는 특수한 버퍼가 필요하므로 다른 3D 버퍼를 생성한다.
	soundBuffer->SetFormat(&waveFormat);
	ThrowIfFailed(soundBuffer->QueryInterface(IID_IDirectSound3DBuffer8, (void**)sound3dBuffer.GetAddressOf()));

	// 복사한 데이터는 더이상 필요 없으므로 삭제한다.
	delete[] waveData;
	waveData = 0;
}

void Sound::Play(const bool loop)
{
	soundBuffer->Play(0, 0, (loop ? 1 : 0));
	isPlay = true;
}

void Sound::Stop(const bool toFirst)
{
	soundBuffer->Stop();
	isPlay = false;

	if (toFirst)
	{
		soundBuffer->SetCurrentPosition(0L);
	}
}

void Sound::SetPosition(const float x, const float y, const float z)
{
	if (sound3dBuffer)
		sound3dBuffer->SetPosition(x, y, z, DS3D_IMMEDIATE);
}

void Sound::SetPosition(const XMFLOAT3& pos)
{
	SetPosition(pos.x, pos.y, pos.z);
}

void Sound::SetBound(const float min, const float max)
{
	sound3dBuffer->SetMinDistance(min, DS3D_IMMEDIATE);
	sound3dBuffer->SetMaxDistance(max, DS3D_IMMEDIATE);
}

void Sound::SetVolume(const UINT32 volume)
{
	this->volume = volume;
	// [0, 100]인 볼륨을 DirectSound에서 사용할 수 있도록 [-10000, 0]으로 변환한다.
	LONG dsbVolume = (LONG)(((float(volume) / 100.0f) - 1.0f) * DSBVOLUME_MIN);
	soundBuffer->SetVolume(dsbVolume);
}

void Sound::SetPan(const float pan)
{
	this->pan = pan;
	// [-1, 1]인 Pan을 DirectSound에서 사용할 수 있도록 [-10000, 10000]으로 변환한다.
	LONG dsbpPan = (LONG)(pan * 10000.0f);
	soundBuffer->SetPan(dsbpPan);
}

void Sound::SetSpeed(float speed)
{
	this->speed = speed;

	DWORD dsbFrequency;
	soundBuffer->GetFrequency(&dsbFrequency);
	dsbFrequency = (DWORD)(speed * dsbFrequency);
	dsbFrequency = std::clamp<DWORD>(dsbFrequency, DSBFREQUENCY_MIN, DSBFREQUENCY_MAX);

	soundBuffer->SetFrequency(dsbFrequency);
}