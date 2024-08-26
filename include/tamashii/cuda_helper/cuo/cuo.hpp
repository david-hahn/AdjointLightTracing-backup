#pragma once
#include <cuda.h>
#include <cuda_runtime.h>
#include <optix.h>

/*! helper function that initializes optix and checks for errors */




















































enum Location {
	
	Host,
	
	Device,
	
	Auto
};
template<Location E>
class MemoryPointer
{
public:
	MemoryPointer(void* aPointer) : mPointer(aPointer) {}
	void* pointer() const { return mPointer; }
private:
	void* mPointer;
};



#define CUDA_CHECK(call)							                \
{									                                \
	cudaError_t rc = call;                                          \
	if (rc != cudaSuccess) {                                        \
		cudaError_t err =  rc; /*cudaGetLastError();*/              \
	}																\
}

#define CUO_INLINE inline
#define CUO_BEGIN_NAMESPACE namespace cuo {
#define CUO_END_NAMESPACE }
#define CUO_USE_NAMESPACE using namespace cuo;