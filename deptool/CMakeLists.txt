project(smokedeptool)

find_package(Qt5Core REQUIRED)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

add_executable(smokedeptool main.cpp)
target_link_libraries(smokedeptool smokebase Qt5::Core)

if (WIN32)
	# Realign the stack, for compatibility with an older ABI.
	if(CMAKE_COMPILER_IS_GNUCXX)
		set_target_properties(smokedeptool PROPERTIES COMPILE_FLAGS -mstackrealign)
	endif()
endif (WIN32)
