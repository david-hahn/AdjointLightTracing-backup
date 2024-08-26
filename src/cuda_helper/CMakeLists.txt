file(GLOB_RECURSE SOURCES "*.h" "*.hpp" "*.c" "*.cpp") 
file(GLOB_RECURSE HEADERS "${INCLUDE_DIR}/tamashii/cuda_helper/*.h" "${INCLUDE_DIR}/tamashii/cuda_helper/*.hpp" 
"${INCLUDE_DIR}/tamashii/cuda_helper/*.c" "${INCLUDE_DIR}/tamashii/cuda_helper/*.cpp")

foreach(FILE ${SOURCES}) 
   # Get the directory of the source file
   get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)
   # Remove common directory prefix to make the group
   string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" PARENT_DIR "${PARENT_DIR}")
   string(REPLACE "${SOURCE_DIR}" "" GROUP "${PARENT_DIR}")
   # Make sure we are using windows slashes
   string(REPLACE "/" "\\" GROUP "${GROUP}")
   set(GROUP "Source Files${GROUP}")
   source_group("${GROUP}" FILES "${FILE}")
endforeach()
foreach(FILE ${HEADERS}) 
   # Get the directory of the source file
   get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)
   # Remove common directory prefix to make the group
   string(REPLACE "${SOURCE_DIR}" "" PARENT_DIR "${PARENT_DIR}")
   string(REPLACE "include/tamashii/cuda_helper" "" GROUP "${PARENT_DIR}")
   # Make sure we are using windows slashes
   string(REPLACE "/" "\\" GROUP "${GROUP}")
   set(GROUP "Header Files${GROUP}")
   source_group("${GROUP}" FILES "${FILE}")
endforeach()

# EXECUTABLE
add_library(${LIB_CUDA_HELPER} STATIC ${SOURCES} ${HEADERS})

# DEPS
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# spdlog
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${LIB_CUDA_HELPER} PRIVATE spdlog::spdlog)
# cuda
find_package(CUDAToolkit REQUIRED)
target_link_libraries(${LIB_CUDA_HELPER} PUBLIC CUDA::cuda_driver CUDA::cudart_static)
# optix
if (NOT DEFINED ENV{OptiX_INSTALL_DIR})
  message(STATUS "Please set the environment variable OptiX_INSTALL_DIR pointing to \"OptiX SDK #.#.#\"; going to use this for finding optix.h")
endif()
find_package(OptiX REQUIRED)
target_link_libraries(${LIB_CUDA_HELPER} PUBLIC OptiX::OptiX)

# INCLUDES
include_directories("${INCLUDE_DIR}")

# COMPILE
set_target_properties(${LIB_CUDA_HELPER} PROPERTIES LINKER_LANGUAGE CXX)