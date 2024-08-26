file(GLOB HEADERS "${INCLUDE_DIR}/tamashii/implementations/*.h" "${INCLUDE_DIR}/tamashii/implementations/*.hpp")
file(GLOB SOURCES "*.h" "*.hpp" "*.c" "*.cpp") 

if(BUILD_PYTHON_BINDINGS)
    file(GLOB HEADERS_BINDINGS "${INCLUDE_DIR}/tamashii/implementations/bindings/*.h" "${INCLUDE_DIR}/tamashii/implementations/bindings/*.hpp")
    file(GLOB SOURCES_BINDINGS "bindings/*.h" "bindings/*.hpp" "bindings/*.c" "bindings/*.cpp") 

    list (APPEND HEADERS ${HEADERS_BINDINGS})
    list (APPEND SOURCES ${SOURCES_BINDINGS})

    # NANOBIND
    find_package(nanobind PATHS "${EXTERNAL_DIR}/nanobind" CONFIG REQUIRED)
endif()

# GROUPING
source_group(TREE "${INCLUDE_DIR}" PREFIX "Header Files" FILES ${HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})

# EXECUTABLE
add_library(${LIB_IMPL} STATIC ${SOURCES} ${HEADERS})
add_library(${APP}::implementations ALIAS ${LIB_IMPL})
set_target_properties(${LIB_IMPL} PROPERTIES FOLDER ${FRAMEWORK_TARGET_FOLDER})

if(BUILD_PYTHON_BINDINGS)
    # INCLUDE
    target_include_directories(${LIB_IMPL} PUBLIC "${EXTERNAL_DIR}/nanobind/include" ${Python_INCLUDE_DIRS})
    # COMPILER
    set_target_properties(${LIB_IMPL} PROPERTIES CXX_VISIBILITY_PRESET hidden)
endif()

# DEPS
add_dependencies(${LIB_IMPL} tamashii::core rvk::rvk)
target_link_libraries(${LIB_IMPL} PRIVATE tamashii::core rvk::rvk)

# COMPILE
set_target_properties(${LIB_IMPL} PROPERTIES LINKER_LANGUAGE CXX)
