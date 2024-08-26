# SOURCES
file(GLOB_RECURSE SOURCES "*.hpp" "*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "platform/(win32|macos|unix)")
if(WIN32)
   list(FILTER SOURCES EXCLUDE REGEX "gui/x11")
   file (GLOB_RECURSE SOURCES_WIN32 "platform/win32/*.*")
   list (APPEND SOURCES ${SOURCES_WIN32})
elseif(APPLE)
   list(FILTER SOURCES EXCLUDE REGEX "gui/x11")
   file (GLOB_RECURSE SOURCES_MACOS "platform/macos/*.*")
   list (APPEND SOURCES ${SOURCES_MACOS})
else(UNIX)
   file (GLOB_RECURSE SOURCES_UNIX "platform/unix/*.*")
   list (APPEND SOURCES ${SOURCES_UNIX})
endif()
# HEADERS
file(GLOB_RECURSE HEADERS "${INCLUDE_DIR}/tamashii/public.hpp" "${INCLUDE_DIR}/tamashii/core/*.*")
# EXTERNALS
file(GLOB EXTERNALS
   # ccli
   "${EXTERNAL_DIR}/ccli/src/ccli.cpp"
   # miniz
   "${EXTERNAL_DIR}/tinyexr/deps/miniz/miniz.c"
   # MikkTSpace
   "${EXTERNAL_DIR}/MikkTSpace/mikktspace.c"
   # imgui
   "${EXTERNAL_DIR}/imgui/imgui.h"
   "${EXTERNAL_DIR}/imgui/imgui_internal.h"
   "${EXTERNAL_DIR}/imgui/imgui.cpp"
   "${EXTERNAL_DIR}/imgui/imgui_draw.cpp"
   "${EXTERNAL_DIR}/imgui/imgui_tables.cpp"
   "${EXTERNAL_DIR}/imgui/imgui_widgets.cpp"
   # imGuizmo
   "${EXTERNAL_DIR}/ImGuizmo/ImGuizmo.cpp"
)
if(WIN32)
   list(APPEND EXTERNALS "${EXTERNAL_DIR}/imgui/backends/imgui_impl_win32.h" "${EXTERNAL_DIR}/imgui/backends/imgui_impl_win32.cpp")
elseif(APPLE)
   list(APPEND EXTERNALS "${EXTERNAL_DIR}/imgui/backends/imgui_impl_osx.h" "${EXTERNAL_DIR}/imgui/backends/imgui_impl_osx.cpp")
endif()

# GROUPING
source_group(TREE "${INCLUDE_DIR}" PREFIX "Header Files" FILES ${HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SOURCES})
source_group(TREE "${EXTERNAL_DIR}" PREFIX "External Files" FILES ${EXTERNALS})

# EXECUTABLE
add_library(${LIB_CORE} STATIC ${SOURCES} ${HEADERS} ${EXTERNALS})
add_library(${APP}::core ALIAS ${LIB_CORE})
set_target_properties(${LIB_CORE} PROPERTIES FOLDER ${FRAMEWORK_TARGET_FOLDER})
set_target_properties(${LIB_CORE} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# INCLUDE
target_include_directories(${LIB_CORE} 
   PRIVATE
   "${EXTERNAL_DIR}/happly"
   "${EXTERNAL_DIR}/tinyply/source"
   "${EXTERNAL_DIR}/tinyobjloader"
   "${EXTERNAL_DIR}/tinyies"
   "${EXTERNAL_DIR}/tinyldt"
   "${EXTERNAL_DIR}/tinyexr"
   "${EXTERNAL_DIR}/tinyexr/deps/miniz"
   "${EXTERNAL_DIR}/MikkTSpace"
   "${EXTERNAL_DIR}/tinygltf"
   "${EXTERNAL_DIR}/nlohmann_json/include"
   "${EXTERNAL_DIR}/stb"
   PUBLIC
   "${EXTERNAL_DIR}/ccli/include"
   "${EXTERNAL_DIR}/glm"
   "${EXTERNAL_DIR}/spdlog/include"
   "${EXTERNAL_DIR}/imgui" "${EXTERNAL_DIR}/imgui/backends"
   "${EXTERNAL_DIR}/ImGuizmo"
   "${EXTERNAL_DIR}/imoguizmo"
)

# DEPS
add_dependencies(${LIB_CORE} spdlog::spdlog)
target_link_libraries(${LIB_CORE} PUBLIC spdlog::spdlog)

if(WIN32)
   target_link_libraries(${LIB_CORE} 
      PRIVATE "winmm.lib" "dinput8.lib" "dxguid.lib" "Shlwapi.lib"
   )
   if(NOT MSVC)
      target_link_libraries(${LIB_CORE} PRIVATE "-limm32" "-ldwmapi")
   endif()
elseif(APPLE)
   target_link_libraries(${LIB_CORE} 
      PRIVATE "-framework Foundation" "-framework AppKit" "-framework GameController"
   )
else(UNIX)
   find_package(X11 REQUIRED)
   find_package(Threads REQUIRED)
   target_link_libraries(${LIB_CORE} PRIVATE "-lxcb -lxcb-util -lxcb-xkb -lxcb-xfixes -lxcb-cursor -lxcb-keysyms -lncurses")
   target_link_libraries(${LIB_CORE} PRIVATE Threads::Threads)
endif()

# COMPILE
set_target_properties(${LIB_CORE} PROPERTIES LINKER_LANGUAGE CXX)
