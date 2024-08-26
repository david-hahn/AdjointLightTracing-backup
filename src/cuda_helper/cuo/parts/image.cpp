#include <tamashii/cuda_helper/cuo/parts/image.hpp>

CUO_USE_NAMESPACE
cuo::Image::Image() : mWidth(0), mHeight(0), mPitch(0), mAllocCapacity(0), mMemoryPointer(nullptr), mChannelFormatDesc()
{}

Image::Image(const size_t aWidth, const size_t aHeight, const cudaChannelFormatDesc aChannelFormatDesc) : Image()
{
	alloc(aWidth, aHeight, aChannelFormatDesc);
}

Image::~Image()
{
	free();
}

void Image::alloc(const size_t aWidth, const size_t aHeight, const cudaChannelFormatDesc aChannelFormatDesc)
{
	mWidth = aWidth; mHeight = aHeight;
	mChannelFormatDesc = aChannelFormatDesc;
	mPitch = aWidth * aChannelFormatDesc.x * aChannelFormatDesc.y * aChannelFormatDesc.z * aChannelFormatDesc.w;
	mAllocCapacity = mPitch * aHeight;
	CUDA_CHECK(cudaMallocArray(&mMemoryPointer, &aChannelFormatDesc, aWidth, aHeight, 0))
}

void Image::free() const
{
	CUDA_CHECK(cudaFreeArray(mMemoryPointer))
}

size_t Image::width() const
{
	return mWidth;
}

size_t Image::height() const
{
	return mHeight;
}

size_t Image::size() const
{
	return mAllocCapacity;
}

size_t Image::pitch() const
{
	return mPitch;
}

cudaArray_t Image::handle() const
{
	return mMemoryPointer;
}
