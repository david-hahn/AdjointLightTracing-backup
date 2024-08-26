include_directories("${INCLUDE_DIR}")
# STATIC LIB
add_subdirectory(core)
if(BUILD_PYTHON_BINDINGS)
    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
    add_subdirectory(core_bindings)
endif()
add_subdirectory(rvk)
add_subdirectory(renderer_vk)

# CUDA HELPER
if(CUDA_HELPER)
    add_subdirectory(cuda_helper)
endif()

# DEFAULT IMPLS
add_subdirectory(default_implementations)
# CUSTOM IMPLS
add_subdirectory(implementations)

# STANDALONE
if(BUILD_TAMASHII)
    add_subdirectory(tamashii)
endif()

