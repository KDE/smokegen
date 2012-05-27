project(smokedeptool)

find_package(Qt4 REQUIRED)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..  ${QT_INCLUDES} )

add_executable(smokedeptool main.cpp)
target_link_libraries(smokedeptool smokebase ${QT_QTCORE_LIBRARY})

if (WIN32)
	# Realign the stack, for compatibility with an older ABI.
	set_target_properties (smokedeptool PROPERTIES COMPILE_FLAGS -mstackrealign)
endif (WIN32)
