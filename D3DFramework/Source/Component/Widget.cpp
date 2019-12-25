#include "pch.h"
#include "Widget.h"
#include "AssetManager.h"
#include "MeshGeometry.h"
#include "Structures.h"

using namespace std::literals;

Widget::Widget(std::string&& name) : Component(std::move(name))
{
	if (mMesh == nullptr)
	{
		mMesh = AssetManager::GetInstance()->FindMesh("WidgetPoint"s);
	}
}

Widget::~Widget() { }

void Widget::SetPosition(std::uint32_t x, std::uint32_t y)
{
	mPosX = x;
	mPosY = y;

	UpdateNumFrames();
}

void Widget::SetSize(std::uint32_t width, std::uint32_t height)
{
	mWidth = width;
	mHeight = height;

	UpdateNumFrames();
}

void Widget::SetAnchor(float x, float y)
{
	mAnchorX = x;
	mAnchorY = y;

	UpdateNumFrames();
}

void Widget::SetMaterial(class Material* material)
{
	mMaterial = material;

	UpdateNumFrames();
}