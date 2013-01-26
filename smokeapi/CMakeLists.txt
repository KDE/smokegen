project(smokeapi)

find_package(Qt5Core REQUIRED)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${QT_INCLUDES} )

add_executable(smokeapi main.cpp)
qt5_use_modules(smokeapi Core)

if (WIN32)
	# Realign the stack, for compatibility with an older ABI.
	if(CMAKE_COMPILER_IS_GNUCXX)
		set_target_properties(smokeapi PROPERTIES COMPILE_FLAGS -mstackrealign)
	endif()
endif (WIN32)

target_link_libraries(smokeapi smokebase ${QT_QTCORE_LIBRARY})
install(    TARGETS smokeapi 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/bin )
