#pragma once

#include <stack>

template<typename T>
class ObjectPool
{
public:
	ObjectPool(size_t size)
	{
		for (size_t i = 0; i < size; ++i)
		{
			T* newObject = new T();
			mStack.push(newObject);
		}
	}

	~ObjectPool()
	{
		while (!mStack.empty())
		{
			T* object = mStack.top();
			mStack.pop();
			delete object;
		}
	}

public:
	T* Acquire()
	{
		if (mStack.empty())
			return nullptr;

		T* object = mStack.top();
		mStack.pop();

		return object;
	}

	void Release(T* object)
	{
		if (object != nullptr)
			mStack.push(object);
	}

	size_t Size() const
	{
		return mStack.size();
	}

private:
	std::stack<T*> mStack;
};