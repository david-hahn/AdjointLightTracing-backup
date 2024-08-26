file(GLOB_RECURSE SOURCES "*.h" "*.hpp" "*.c" "*.cpp") 
file(GLOB_RECURSE HEADERS "${INCLUDE_DIR}/tamashii/renderer_vk/*.h" "${INCLUDE_DIR}/tamashii/renderer_vk/*.hpp")

# GROUPING
source_group(TREE "${INCLUDE_DIR}" PREFIX "Header Files" FILES ${HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})
source_group(TREE "${EXTERNAL_DIR}" PREFIX "External Files" FILES ${EXTERNALS})

# EXECUTABLE
add_library(${LIB_RENDERER_VK} STATIC ${SOURCES} ${HEADERS} ${EXTERNALS})
add_library(${APP}::vkrenderer ALIAS ${LIB_RENDERER_VK})
set_target_properties(${LIB_RENDERER_VK} PROPERTIES FOLDER ${FRAMEWORK_TARGET_FOLDER})

# DEPS
# tamashii
add_dependencies(${LIB_RENDERER_VK} tamashii::core rvk::rvk)
target_link_libraries(${LIB_RENDERER_VK} PUBLIC tamashii::core rvk::rvk)

# COMPILE
set_target_properties(${LIB_RENDERER_VK} PROPERTIES LINKER_LANGUAGE CXX)
