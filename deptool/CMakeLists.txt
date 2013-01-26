project(smokedeptool)

find_package(Qt5Core REQUIRED)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..  ${QT_INCLUDES} )

add_executable(smokedeptool main.cpp)
qt5_use_modules(smokedeptool Core)
target_link_libraries(smokedeptool smokebase ${QT_QTCORE_LIBRARY})

if (WIN32)
	# Realign the stack, for compatibility with an older ABI.
	if(CMAKE_COMPILER_IS_GNUCXX)
		set_target_properties(smokedeptool PROPERTIES COMPILE_FLAGS -mstackrealign)
	endif()
endif (WIN32)
