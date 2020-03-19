#include "pch.h"
#include "WavLoader.h"
#include "Sound.h"

std::unique_ptr<Sound> WavLoader::LoadWave(IDirectSound8* d3dSound, const SoundInfo& soundInfo)
{
	auto[objectName, fileName, soundType] = soundInfo;

	FILE* filePtr = nullptr;
	if (fopen_s(&filePtr, fileName.c_str(), "rb") != 0)
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << fileName.c_str() << " Wave File not Found" << std::endl;
#endif
		return  nullptr;
	}

	WaveHeaderType waveFileHeader;
	if (fread(&waveFileHeader, sizeof(waveFileHeader), 1, filePtr) == -1)
		return nullptr;


	// RIFF �������� Ȯ���Ѵ�.
	if ((waveFileHeader.chunkId[0] != 'R') || (waveFileHeader.chunkId[1] != 'I') ||
		(waveFileHeader.chunkId[2] != 'F') || (waveFileHeader.chunkId[3] != 'F'))
		return nullptr;

	// Wave �������� Ȯ���Ѵ�.
	if ((waveFileHeader.format[0] != 'W') || (waveFileHeader.format[1] != 'A') ||
		(waveFileHeader.format[2] != 'V') || (waveFileHeader.format[3] != 'E'))
		return nullptr;

	// fmt �������� Ȯ���Ѵ�.
	if ((waveFileHeader.subChunkId[0] != 'f') || (waveFileHeader.subChunkId[1] != 'm') ||
		(waveFileHeader.subChunkId[2] != 't') || (waveFileHeader.subChunkId[3] != ' '))
		return nullptr;

	/*
	// ����� ������ WAVE_FORMAT_PCM���� Ȯ���Ѵ�.
	if (waveFileHeader.audioFormat != WAVE_FORMAT_PCM)
		return nullptr;

	// ���׷��� �������� ��ϵǾ� �ִ��� Ȯ���Ѵ�.
	if (waveFileHeader.numChannels != 2)
		return nullptr;

	// 44.1kHz ������ ��ϵǾ����� Ȯ���Ѵ�.
	if (waveFileHeader.sampleRate != 44100)
		return nullptr;

	// 16��Ʈ�� ��ϵǾ� �ִ��� Ȯ���Ѵ�.
	if (waveFileHeader.bitsPerSample != 16)
		return nullptr;
	*/

	// data���� Ȯ���Ѵ�.
	if ((waveFileHeader.dataChunkId[0] != 'd') || (waveFileHeader.dataChunkId[1] != 'a') ||
		(waveFileHeader.dataChunkId[2] != 't') || (waveFileHeader.dataChunkId[3] != 'a'))
	{
#if defined(DEBUG) || defined(_DEBUG)
		std::cout << fileName.c_str() << " Not Match Data" << std::endl;
#endif
		return nullptr;
	}

	std::unique_ptr<Sound> sound = std::make_unique<Sound>(std::move(objectName));

	switch (soundType)
	{
	case SoundType::Sound2D:
		sound->CreateSoundBuffer2D(d3dSound, filePtr, waveFileHeader);
		break;
	case SoundType::Sound3D:
		sound->CreateSoundBuffer3D(d3dSound, filePtr, waveFileHeader);
		break;
	}

	return sound;
}