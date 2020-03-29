#include <Component/Component.h>

Component::Component(std::string&& name) : mName(std::move(name)) 
{
	uid = currUID++; 
}

Component::~Component() { }

bool Component::operator==(const Component& rhs)
{
	return uid == rhs.GetUID();
}

bool Component::operator==(const std::string& str)
{
	if (mName.compare(str) == 0)
		return true;
	return false;
}

std::string Component::ToString() const
{
	return std::to_string(uid) + R"( )" + mName;
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
	mNumFramesDirty = max(0, mNumFramesDirty);
}

std::string Component::GetName() const
{
	return mName; 
}

UINT64 Component::GetUID() const
{
	return uid; 
}