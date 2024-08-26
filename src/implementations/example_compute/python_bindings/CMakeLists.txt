# This name has to match the module name set in the 'NB_MODULE()' macro
set(PYCOMPUTE pycompute)

# Get the sources starting from the parent directory
file(GLOB_RECURSE SOURCES "../*.h" "../*.hpp" "../*.c" "../*.cpp")

# Exclude the entry point of the main application
list(FILTER SOURCES EXCLUDE REGEX "../main.cpp")

# GROUPING
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/.." PREFIX "Source Files" FILES ${SOURCES})

# nanobind already included by core_bindings
nanobind_add_module(
  ${PYCOMPUTE}
  NB_STATIC # Build static libnanobind (the extension module itself remains a shared library)
  ${SOURCES}
)

# The name of the nanobind library target depends on the config above (NB_STATIC, NB_SHARED, LTO, STABLE_API,...)
set_target_properties(nanobind-static PROPERTIES FOLDER ${FRAMEWORK_EXTERNAL_FOLDER})
set_target_properties(${PYCOMPUTE} PROPERTIES FOLDER ${FRAMEWORK_EXAMPLE_FOLDER})

# DEPS
target_link_libraries(${PYCOMPUTE} PRIVATE tamashii::core tamashii::core_bindings tamashii::vkrenderer tamashii::implementations)
add_dependencies(${PYCOMPUTE} tamashii::core tamashii::core_bindings tamashii::vkrenderer tamashii::implementations)

# COMPILER
set_target_properties(${PYCOMPUTE} PROPERTIES CXX_VISIBILITY_PRESET hidden)

# INSTALL
install(TARGETS ${PYCOMPUTE} RUNTIME DESTINATION bin LIBRARY DESTINATION bin ARCHIVE DESTINATION lib)
install(FILES example.py DESTINATION bin RENAME ${PYCOMPUTE}.py)
