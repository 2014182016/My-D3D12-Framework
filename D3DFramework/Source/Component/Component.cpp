#include "pch.h"
#include "Component.h"

Component::Component(std::string&& name) : mName(std::move(name)) { mUID = currUID++; }

Component::~Component() { }

bool Component::operator==(const Component& rhs)
{
	return mUID == rhs.GetUID();
}

bool Component::operator==(const std::string& str)
{
	if (mName.compare(str) == 0)
		return true;
	return false;
}

std::string Component::ToString() const
{
	std::string blank = " ";

	return std::to_string(mUID) + blank + mName;
}

bool Component::IsUpdate() const
{
	return mNumFramesDirty > 0;
}

void Component::UpdateNumFrames()
{
	mNumFramesDirty = NUM_FRAME_RESOURCES;
}

void Component::DecreaseNumFrames()
{
	--mNumFramesDirty;
	mNumFramesDirty = std::max<int>(0, mNumFramesDirty);
}
