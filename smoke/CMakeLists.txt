IF(CMAKE_CXX_FLAGS MATCHES "-fvisibility")
    ADD_DEFINITIONS(-DGCC_VISIBILITY)
ENDIF(CMAKE_CXX_FLAGS MATCHES "-fvisibility")

set(SMOKE_VERSION_MAJOR 3)
set(SMOKE_VERSION_MINOR 0)
set(SMOKE_VERSION_PATCH 0)
set(SMOKE_VERSION ${SMOKE_VERSION_MAJOR}.${SMOKE_VERSION_MINOR}.${SMOKE_VERSION_PATCH})

add_definitions(-DBASE_SMOKE_BUILDING)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/..)

set (SMOKEBASE_SRC smokebase.cpp
                   smokemanager.cpp
                   smokeutils.cpp)

set (SMOKE_HEADERS marshall.h
                   pointermap.h
                   quickmarshall.h
                   smoke.h
                   smokemanager.h
                   smokeutils.h)

add_library(smokebase SHARED ${SMOKEBASE_SRC})
target_link_libraries(smokebase)
if (WIN32)
    target_link_libraries(smokebase kernel32)
else (WIN32)
    target_link_libraries(smokebase dl)
endif (WIN32)
set_target_properties(smokebase PROPERTIES 
                                VERSION ${SMOKE_VERSION}
                                SOVERSION ${SMOKE_VERSION_MAJOR})

include(MacroWriteBasicCMakeVersionFile)
macro_write_basic_cmake_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SmokeVersionConfig.cmake
    ${SMOKE_VERSION_MAJOR}
    ${SMOKE_VERSION_MINOR}
    ${SMOKE_VERSION_PATCH}
)

install(TARGETS smokebase
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
        RUNTIME DESTINATION bin)

if (WIN32)
    # Realign the stack, for compatibility with an older ABI.
    set_target_properties (smokebase PROPERTIES COMPILE_FLAGS -mstackrealign)

    # Get rid of the "lib" prefix on archives/DLLs in Windows.
    set_target_properties(smokebase PROPERTIES PREFIX "" IMPORT_PREFIX "")
endif (WIN32)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SmokeVersionConfig.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/smoke/cmake)
install(FILES ${SMOKE_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/smoke )
