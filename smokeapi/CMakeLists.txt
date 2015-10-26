project(smokeapi)

find_package(Qt5Core REQUIRED)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

add_executable(smokeapi main.cpp)

if (WIN32)
	# Realign the stack, for compatibility with an older ABI.
	if(CMAKE_COMPILER_IS_GNUCXX)
		set_target_properties(smokeapi PROPERTIES COMPILE_FLAGS -mstackrealign)
	endif()
endif (WIN32)

target_link_libraries(smokeapi smokebase Qt5::Core)
install(    TARGETS smokeapi 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/bin )
