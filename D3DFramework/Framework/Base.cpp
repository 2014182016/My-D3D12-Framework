#include "pch.h"
#include "Base.h"

Base::Base(std::string name) : mName(name) { mUID = currUID++; }

Base::~Base() { }

bool Base::operator==(const Base& rhs)
{
	return mUID == rhs.GetUID();
}

bool Base::operator==(const std::string& str)
{
	if (mName.compare(str) == 0)
		return true;
	return false;
}

std::string Base::ToString() const
{
	std::string blank = " ";

	return std::to_string(mUID) + blank + mName;
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