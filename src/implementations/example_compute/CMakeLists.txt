file(GLOB_RECURSE SOURCES "*.h" "*.hpp" "*.c" "*.cpp")

# Exclude files only used for bindings
list(FILTER SOURCES EXCLUDE REGEX "python_bindings/*.*")

# GROUPING
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})

set(APP "example_compute")
# EXECUTABLE
if(WIN32)
	add_executable(${APP} WIN32 ${SOURCES})	
   # set working dir and startup project
   set_target_properties(${APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
elseif(APPLE)
   add_executable(${APP} MACOSX_BUNDLE ${SOURCES})
   set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY XCODE_STARTUP_PROJECT ${APP})
elseif(UNIX)
	add_executable(${APP} ${SOURCES})	
endif()
set_target_properties(${APP} PROPERTIES FOLDER ${FRAMEWORK_EXAMPLE_FOLDER})

# Add build target for the bindings
if(BUILD_PYTHON_BINDINGS)
   add_subdirectory(python_bindings)
endif()

# DEPENDENCIES
# tamashii
target_link_libraries(${APP} PRIVATE tamashii::core tamashii::vkrenderer tamashii::implementations rvk::rvk)
add_dependencies(${APP} tamashii::core tamashii::vkrenderer tamashii::implementations rvk::rvk)
