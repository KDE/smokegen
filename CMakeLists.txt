cmake_minimum_required(VERSION 2.8.11)
project(smokegenerator)

find_package(Qt5Core REQUIRED)
find_package(Qt5Xml REQUIRED)

find_package(LLVM REQUIRED CONFIG)
link_directories(${LLVM_LIBRARY_DIRS})

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(AddLLVM)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake )
set(SMOKE_VERSION_MAJOR 3)
set(SMOKE_VERSION_MINOR 0)
set(SMOKE_VERSION_PATCH 0)
set(SMOKE_VERSION ${SMOKE_VERSION_MAJOR}.${SMOKE_VERSION_MINOR}.${SMOKE_VERSION_PATCH})


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

if (NOT LIB_INSTALL_DIR)
    set (LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
endif (NOT LIB_INSTALL_DIR)

include_directories(${CMAKE_BINARY_DIR})

set(generator_SRC
    astconsumer.cpp
    astvisitor.cpp
    frontendaction.cpp
    defaultargvisitor.cpp
    main.cpp
    options.cpp
    ppcallbacks.cpp
    type.cpp
)

# force RPATH so that the binary is usable from within the build tree
set (CMAKE_SKIP_BUILD_RPATH FALSE)
set (CMAKE_SKIP_RPATH FALSE)
# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

get_target_property(QtCore_location Qt5::Core LOCATION)
GET_FILENAME_COMPONENT(QtCore_lib_dir ${QtCore_location} PATH)
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES ${QtCore_lib_dir} isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${QtCore_lib_dir}")
ENDIF("${isSystemDir}" STREQUAL "-1")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in config.h @ONLY )

# Embed the clang header into the binary:
file(GLOB BUILTINS_HEADERS "${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_PACKAGE_VERSION}/include/*.h")
foreach(BUILTIN_HEADER ${BUILTINS_HEADERS})
    file(READ ${BUILTIN_HEADER} BINARY_DATA HEX)
    string(REGEX REPLACE "(..)" "\\\\x\\1" BINARY_DATA "${BINARY_DATA}")
    string(REPLACE "${LLVM_INSTALL_PREFIX}/lib/clang/${LLVM_VERSION}/include/" "/builtins/" FN "${BUILTIN_HEADER}"  )
    set(EMBEDDED_DATA "${EMBEDDED_DATA} { \"${FN}\" , \"${BINARY_DATA}\" } , ")
endforeach()

configure_file(embedded_includes.h.in embedded_includes.h)

add_executable(smokegen ${generator_SRC})
target_link_libraries(smokegen Qt5::Core Qt5::Xml
    -Wl,--start-group
    clangAST
    clangAnalysis
    clangBasic
    clangDriver
    clangEdit
    clangFrontend
    clangFrontendTool
    clangLex
    clangParse
    clangSema
    clangEdit
    clangASTMatchers
    clangRewrite
    clangRewriteFrontend
    clangStaticAnalyzerFrontend
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangSerialization
    clangToolingCore
    clangTooling
    -Wl,--end-group
    ${LLVM_AVAILABLE_LIBS}
)

set_target_properties(smokegen PROPERTIES COMPILE_DEFINITIONS __GENERATOR_BUILDING ENABLE_EXPORTS TRUE)
set(LLVM_REQUIRES_RTTI ON)
llvm_update_compile_flags(smokegen)
target_compile_features(smokegen PRIVATE cxx_nullptr)
set_property(
    SOURCE
        astconsumer.cpp
        frontendaction.cpp
        main.cpp
        ppcallbacks.cpp
    APPEND
    PROPERTY COMPILE_FLAGS -fno-rtti
)

install(TARGETS smokegen LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
if (WIN32)
	# Realign the stack, for compatibility with an older ABI
	if(CMAKE_COMPILER_IS_GNUCXX)
		set_target_properties(smokegen PROPERTIES COMPILE_FLAGS -mstackrealign)
	endif()

    # Get rid of the "lib" prefix on archives/DLLs in Windows.
    set_target_properties(smokegen PROPERTIES PREFIX "" IMPORT_PREFIX "")
endif (WIN32)
install(FILES options.h type.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/smokegen)
install(FILES smoke.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include )

add_subdirectory(cmake)
add_subdirectory(generators)
add_subdirectory(smokeapi)
add_subdirectory(smokebase)
add_subdirectory(deptool)
