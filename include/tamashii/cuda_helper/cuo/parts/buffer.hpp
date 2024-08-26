#pragma once
#include <vector>
#include <tamashii/cuda_helper/cuo/cuo.hpp>

CUO_BEGIN_NAMESPACE

template<Location E>
class Buffer {
public:
													Buffer();
													Buffer(size_t aSize);
													~Buffer();

	void											alloc(size_t aSize);
	void											set(int aValue = 0, size_t aOffset = 0, size_t aCount = 0) const;
	void											free() const;

	MemoryPointer<E>								memoryPointer(size_t aOffset = 0);
	size_t											size() const;

private:
	void*											mMemoryPointer;
	size_t											mAllocCapacity;
};


CUO_END_NAMESPACE
