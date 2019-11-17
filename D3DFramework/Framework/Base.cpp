#include "pch.h"
#include "Base.h"

Base::Base(std::string name) : mName(name) { }

Base::~Base() { }

bool Base::operator==(const Base& rhs)
{
	int result = mName.compare(rhs.mName);

	if (result == 0)
		return true;
	return false;
}

bool Base::operator==(const std::string& str)
{
	if (mName.compare(str) == 0)
		return true;
	return false;
}

std::string Base::ToString() const
{
	return mName;
}

bool Base::IsUpdate() const
{
	return mNumFramesDirty > 0;
}

void Base::UpdateNumFrames()
{
	mNumFramesDirty = NUM_FRAME_RESOURCES;
}

void Base::DecreaseNumFrames()
{
	--mNumFramesDirty;
	mNumFramesDirty = std::max<int>(0, mNumFramesDirty);
}