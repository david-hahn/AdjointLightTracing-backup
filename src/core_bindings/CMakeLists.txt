# SOURCES
file(GLOB_RECURSE SOURCES "*.hpp" "*.cpp")
file(GLOB_RECURSE HEADERS "${INCLUDE_DIR}/tamashii/bindings/*.hpp")

# GROUPING
source_group(TREE "${INCLUDE_DIR}" PREFIX "Header Files" FILES ${HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})

# NANOBIND
find_package(nanobind PATHS "${EXTERNAL_DIR}/nanobind" CONFIG REQUIRED)
nanobind_build_library(nanobind-static STATIC)

# EXECUTABLE
add_library(${LIB_CORE_BINDINGS} STATIC ${SOURCES} ${HEADERS})
add_library(${APP}::core_bindings ALIAS ${LIB_CORE_BINDINGS})
set_target_properties(${LIB_CORE_BINDINGS} PROPERTIES FOLDER ${FRAMEWORK_TARGET_FOLDER})

# INCLUDE
target_include_directories(${LIB_CORE_BINDINGS}
  PUBLIC
  "${EXTERNAL_DIR}/nanobind/include"
  ${Python_INCLUDE_DIRS}
)

# DEPS
target_link_libraries(${LIB_CORE_BINDINGS} PRIVATE tamashii::core PUBLIC nanobind-static)
add_dependencies(${LIB_CORE_BINDINGS} nanobind-static tamashii::core)

# COMPILE
set_target_properties(${LIB_CORE_BINDINGS} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${LIB_CORE_BINDINGS} PROPERTIES CXX_VISIBILITY_PRESET hidden)
