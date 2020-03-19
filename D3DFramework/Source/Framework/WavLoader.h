#pragma once

#include "pch.h"
#include "Structure.h"

class WavLoader
{
public:
	static 	std::unique_ptr<class Sound> LoadWave(IDirectSound8* d3dSound, const SoundInfo& soundInfo);
};