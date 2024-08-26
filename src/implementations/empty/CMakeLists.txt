file(GLOB_RECURSE SOURCES "*.h" "*.hpp" "*.c" "*.cpp")

# GROUPING
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})

set(APP "empty")
# EXECUTABLE
if(WIN32)
	add_executable(${APP} WIN32 ${SOURCES})	
   # set working dir and startup project
   set_target_properties(${APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
elseif(APPLE)
   add_executable(${APP} MACOSX_BUNDLE ${SOURCES})
   set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY XCODE_STARTUP_PROJECT ${APP})
else(UNIX)
	add_executable(${APP} ${SOURCES})	
endif()
set_target_properties(${APP} PROPERTIES FOLDER ${FRAMEWORK_EXAMPLE_FOLDER})

# INCLUDES
include_directories("${INCLUDE_DIR}")
# DEPENDENCIES
# tamashii
target_link_libraries(${APP} PRIVATE tamashii::core tamashii::vkrenderer tamashii::implementations rvk::rvk)
add_dependencies(${APP} tamashii::core tamashii::vkrenderer tamashii::implementations rvk::rvk)

