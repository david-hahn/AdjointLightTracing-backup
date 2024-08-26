file(GLOB_RECURSE SOURCES "*.h" "*.hpp" "*.c" "*.cpp")

# Exclude files only used for bindings
list(FILTER SOURCES EXCLUDE REGEX "python_bindings/*.*")

# GROUPING
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})

if(WIN32)
   add_executable(${APP} WIN32 ${SOURCES})
   set_target_properties(${APP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
   set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${APP})
elseif(APPLE)
   add_executable(${APP} MACOSX_BUNDLE ${SOURCES})
   set_target_properties(${APP} PROPERTIES XCODE_GENERATE_SCHEME TRUE CMAKE_XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
   set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY XCODE_STARTUP_PROJECT ${APP})
elseif(UNIX)
   add_executable(${APP} ${SOURCES})
endif()

# Add build target for the bindings
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python_bindings)
endif()

# DEPS
target_link_libraries(${APP} PRIVATE tamashii::core tamashii::vkrenderer tamashii::implementations)
add_dependencies(${APP} tamashii::core tamashii::vkrenderer tamashii::implementations)

# INSTALL
if(WIN32)
   install(TARGETS ${APP} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
   install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION bin/assets)
   install(FILES $<TARGET_RUNTIME_DLLS:${APP}> DESTINATION bin)
elseif(APPLE)
elseif(UNIX)
   install(TARGETS ${APP} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
   install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION bin/assets)
endif()
