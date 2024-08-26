file(GLOB_RECURSE ASSET_FILES *)

# this is just to keep the folder structure
foreach(FILE ${ASSET_FILES}) 
    # Get the directory of the source file
	get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    source_group("${GROUP}" FILES "${FILE}")
endforeach()

add_custom_target(
	${ASSETS}
	SOURCES ${ASSET_FILES})