#pragma once
#include <tamashii/cuda_helper/cuo/cuo.hpp>
#include <tamashii/cuda_helper/cuo/parts/buffer.hpp>
#include <tamashii/cuda_helper/cuo/parts/image.hpp>

CUO_BEGIN_NAMESPACE
class Stream
{
public:
	Stream() : mStream(nullptr) {}

	template<Location Src, Location Dst>
	void copy(MemoryPointer<Src> aSrc, MemoryPointer<Dst> aDst, const size_t aCount)
	{
		cudaMemcpyKind memcpyKind = cudaMemcpyDefault;
		if constexpr ((Src == Host) && (Dst == Host)) memcpyKind = cudaMemcpyHostToHost;
		else if constexpr ((Src == Host) && (Dst == Device)) memcpyKind = cudaMemcpyHostToDevice;
		else if constexpr ((Src == Device) && (Dst == Host)) memcpyKind = cudaMemcpyDeviceToHost;
		else if constexpr ((Src == Device) && (Dst == Device)) memcpyKind = cudaMemcpyDeviceToDevice;
		cudaMemcpyAsync(aDst.pointer(), aSrc.pointer(), aCount, memcpyKind, mStream);
	}
private:
	cudaStream_t mStream;
};

inline void test()
{
	Buffer<Host> buff1;
	buff1.alloc(10);
	Buffer<Device> buff2;
	buff1.alloc(10);
	const MemoryPointer bp1 = buff1.memoryPointer(0);
	const MemoryPointer bp2 = buff2.memoryPointer(0);

	Stream defaultStream;
	defaultStream.copy(bp1, bp2, 10);

	
	
	
	
	
	
}
CUO_END_NAMESPACE