file(GLOB_RECURSE SOURCES "src/*.h" "src/*.hpp" "src/*.c" "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp" "include/*.c" "include/*.cpp" "include/*.inl")

# GROUPING
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${HEADERS})

# EXECUTABLE
add_library(${LIB_RVK} STATIC ${SOURCES} ${HEADERS})
add_library(${LIB_RVK}::${LIB_RVK} ALIAS ${LIB_RVK})
set_target_properties(${LIB_RVK} PROPERTIES FOLDER ${FRAMEWORK_TARGET_FOLDER})

# DEPS
# glslang
target_link_libraries(${LIB_RVK}
	PRIVATE glslang MachineIndependent GenericCodeGen OSDependent SPIRV glslang-default-resource-limits
)
add_dependencies(${LIB_RVK} glslang MachineIndependent GenericCodeGen OSDependent glslang-default-resource-limits)

if(WIN32)
elseif(APPLE)
else(UNIX)
   find_package(Threads REQUIRED)
   target_link_libraries(${LIB_RVK} PRIVATE Threads::Threads)
   target_link_libraries(${LIB_RVK} PRIVATE  ${CMAKE_DL_LIBS})
endif()

# INCLUDES
target_include_directories(${LIB_RVK} PUBLIC "${EXTERNAL_DIR}/vulkan-headers/include" "${EXTERNAL_DIR}/glslang/include" "${SOURCE_DIR}/src/rvk/include")

# COMPILER
target_compile_definitions(${LIB_RVK} PRIVATE _R_VK_EXPORT_DLL)

