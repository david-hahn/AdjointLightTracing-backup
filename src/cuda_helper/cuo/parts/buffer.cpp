#include <tamashii/cuda_helper/cuo/parts/buffer.hpp>

CUO_USE_NAMESPACE

template <Location E>
Buffer<E>::Buffer(): mMemoryPointer(nullptr), mAllocCapacity(0)
{
}

template <Location E>
Buffer<E>::Buffer(const size_t aSize)
{
	alloc(aSize);
}

template <Location E>
Buffer<E>::~Buffer()
{
	free();
}

template <Location E>
void Buffer<E>::alloc(const size_t aSize)
{
	
	if constexpr (E == Host) {
		CUDA_CHECK(cudaMallocHost(&mMemoryPointer, aSize))
	}
	else if constexpr (E == Device) {
		CUDA_CHECK(cudaMalloc(&mMemoryPointer, aSize));
	}
	else if constexpr (E == Auto) {
		CUDA_CHECK(cudaMallocManaged(&mMemoryPointer, aSize));
	}
}

template <Location E>
void Buffer<E>::set(const int aValue, const size_t aOffset, size_t aCount) const
{
	static_assert(aOffset + aCount < mAllocCapacity);
	aCount = aCount ? aCount : (mAllocCapacity - aOffset);
	CUDA_CHECK(cudaMemset(mMemoryPointer + aOffset, aValue, aCount))
}

template <Location E>
void Buffer<E>::free() const
{
	
	if constexpr (E == Host) {
		CUDA_CHECK(cudaFreeHost(mMemoryPointer))
	}
	else if constexpr (E == Device) {
		CUDA_CHECK(cudaFree(mMemoryPointer))
	}
	else if constexpr (E == Auto) {
		CUDA_CHECK(cudaFree(mMemoryPointer))
	}
}

template <Location E>
MemoryPointer<E> Buffer<E>::memoryPointer(const size_t aOffset)
{
	return MemoryPointer<E>(static_cast<char*>(mMemoryPointer) + aOffset);
}

template <Location E>
size_t Buffer<E>::size() const
{
	return mAllocCapacity;
}
