#pragma once
#include <tamashii/cuda_helper/cuo/cuo.hpp>

CUO_BEGIN_NAMESPACE
class Image {
public:
													Image();
													Image(size_t aWidth, size_t aHeight, cudaChannelFormatDesc aChannelFormatDesc);
													~Image();

	void											alloc(size_t aWidth, size_t aHeight, cudaChannelFormatDesc aChannelFormatDesc);
	void											free() const;

	size_t											width() const;
	size_t											height() const;
	size_t											size() const;
	size_t											pitch() const;
	cudaArray_t										handle() const;
private:
	size_t											mWidth;
	size_t											mHeight;
	size_t											mPitch;
	size_t											mAllocCapacity;
	cudaArray_t										mMemoryPointer;
	cudaChannelFormatDesc							mChannelFormatDesc;
};
CUO_END_NAMESPACE