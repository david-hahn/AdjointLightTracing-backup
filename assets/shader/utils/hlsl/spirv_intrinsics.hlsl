#ifndef SPIRV_INTRINSICS_HLSL
#define SPIRV_INTRINSICS_HLSL

namespace spirv
{
    enum class scope : uint {
        CrossDevice = 0,
        Device = 1,
        Workgroup = 2,
        Subgroup = 3,
        Invocation = 4,
        QueueFamily = 5,
        QueueFamilyKHR = 6,
        ShaderCallKHR = 7
    };

    enum memory_semantics : uint {
        None = 0x0,
        Acquire = 0x2,
        Release = 0x4,
        AcquireRelease = 0x8,
        SequentiallyConsistent = 0x10,
        UniformMemory = 0x40,
        SubgroupMemory = 0x80,
        WorkgroupMemory = 0x100,
        CrossWorkgroupMemory = 0x200,
        AtomicCounterMemory = 0x400,
        ImageMemory = 0x800,
        OutputMemory = 0x1000,
        OutputMemoryKHR = 0x1000,
        MakeAvailable = 0x2000,
        MakeAvailableKHR = 0x2000,
        MakeVisible = 0x4000,
        MakeVisibleKHR = 0x4000,
        Volatile = 0x8000
    };

    [[vk::ext_capability(/* AtomicFloat32AddEXT */ 6033)]]
    [[vk::ext_extension("SPV_EXT_shader_atomic_float_add")]]
    [[vk::ext_instruction(/* OpAtomicFAddEXT */ 6035)]]
    float opAtomicFAddEXT([[vk::ext_reference]] float ptr, scope scope, uint mem_semantics, float value);

    
    

    [[vk::ext_capability(/* AtomicFloat64AddEXT */ 6034)]]
    [[vk::ext_extension("SPV_EXT_shader_atomic_float_add")]]
    [[vk::ext_instruction(/* OpAtomicFAddEXT */ 6035)]]
    double opAtomicFAddEXT([[vk::ext_reference]] double ptr, scope scope, uint mem_semantics, double value);
}
#endif
