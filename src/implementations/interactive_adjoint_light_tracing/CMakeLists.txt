file(GLOB_RECURSE SOURCES "*.h" "*.hpp" "*.c" "*.cpp")

# Exclude files only used for bindings
list(FILTER SOURCES EXCLUDE REGEX "python_bindings/*.*")

# GROUPING
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})

set(APP "inter_adj_light_trace")
set(APP_SH "inter_adj_light_trace_sh")

foreach(TARGET IN LISTS APP APP_SH)
	# exe
	if(WIN32)
		add_executable(${TARGET} WIN32 ${SOURCES})	
		# set working dir and startup project
		set_target_properties(${TARGET} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	elseif(APPLE)
	else(UNIX)
		add_executable(${TARGET} ${SOURCES})	
	endif()

	# OPTIONS
	if(${TARGET} STREQUAL ${APP_SH})
		target_compile_definitions(${TARGET} PUBLIC IALT_USE_SPHERICAL_HARMONICS)
	endif()

	# OPTIONS
	option(${TARGET}_USE_HARDCODED_TARGETS "${TARGET}: Use hardcoded lighting targets (for testing)" OFF)
	if(${TARGET}_USE_HARDCODED_TARGETS)
		target_compile_definitions(${TARGET} PUBLIC IALT_USE_HARDCODED_TARGETS)
	endif()

	option(${TARGET}_USE_QUADRATIC_INTENSITY_OPT "${TARGET}: Use quadratic intensity parameterisation during optimisation" ON)
	if(${TARGET}_USE_QUADRATIC_INTENSITY_OPT)
		target_compile_definitions(${TARGET} PUBLIC IALT_USE_QUADRATIC_INTENSITY_OPT)
	endif()

	# DEPENDENCIES
	# tamashii
	target_link_libraries(${TARGET} PRIVATE ${LIB_ENGINE} ${LIB_RENDERER_VK} ${LIB_GUI} ${LIB_IMPL})
	add_dependencies(${TARGET} ${LIB_ENGINE} ${LIB_RENDERER_VK} ${LIB_GUI} ${LIB_IMPL})

	# INCLUDES
	target_include_directories(${TARGET} PRIVATE "${INCLUDE_DIR}" "${EXTERNAL_DIR}/eigen")

	# INSTALL
	install(TARGETS ${TARGET} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
endforeach()

# Add build target for the bindings
if(BUILD_PYTHON_BINDINGS)
   add_subdirectory(python_bindings)
endif()

