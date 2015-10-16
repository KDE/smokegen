cmake_minimum_required(VERSION 2.8.9)
project(smokegenerator)

find_package(Qt4 REQUIRED)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

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

include_directories(${QT_INCLUDES} ${CMAKE_BINARY_DIR})

set(generator_SRC
    astconsumer.cpp
    astvisitor.cpp
    frontendaction.cpp
    main.cpp
    options.cpp
    type.cpp
)

# force RPATH so that the binary is usable from within the build tree
set (CMAKE_SKIP_BUILD_RPATH FALSE)
set (CMAKE_SKIP_RPATH FALSE)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in config.h @ONLY )

add_executable(smokegen ${generator_SRC})
target_link_libraries(smokegen ${QT_QTCORE_LIBRARY} ${QT_QTXML_LIBRARY}
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
    LLVMLTO
    LLVMObjCARCOpts
    LLVMLinker
    LLVMBitWriter
    LLVMIRReader
    LLVMAsmParser
    LLVMR600CodeGen
    LLVMipo
    LLVMVectorize
    LLVMR600AsmParser
    LLVMR600Desc
    LLVMR600Info
    LLVMR600AsmPrinter
    LLVMSystemZDisassembler
    LLVMSystemZCodeGen
    LLVMSystemZAsmParser
    LLVMSystemZDesc
    LLVMSystemZInfo
    LLVMSystemZAsmPrinter
    LLVMHexagonDisassembler
    LLVMHexagonCodeGen
    LLVMHexagonDesc
    LLVMHexagonInfo
    LLVMNVPTXCodeGen
    LLVMNVPTXDesc
    LLVMNVPTXInfo
    LLVMNVPTXAsmPrinter
    LLVMCppBackendCodeGen
    LLVMCppBackendInfo
    LLVMMSP430CodeGen
    LLVMMSP430Desc
    LLVMMSP430Info
    LLVMMSP430AsmPrinter
    LLVMXCoreDisassembler
    LLVMXCoreCodeGen
    LLVMXCoreDesc
    LLVMXCoreInfo
    LLVMXCoreAsmPrinter
    LLVMMipsDisassembler
    LLVMMipsCodeGen
    LLVMMipsAsmParser
    LLVMMipsDesc
    LLVMMipsInfo
    LLVMMipsAsmPrinter
    LLVMAArch64Disassembler
    LLVMAArch64CodeGen
    LLVMAArch64AsmParser
    LLVMAArch64Desc
    LLVMAArch64Info
    LLVMAArch64AsmPrinter
    LLVMAArch64Utils
    LLVMARMDisassembler
    LLVMARMCodeGen
    LLVMARMAsmParser
    LLVMARMDesc
    LLVMARMInfo
    LLVMARMAsmPrinter
    LLVMPowerPCDisassembler
    LLVMPowerPCCodeGen
    LLVMPowerPCAsmParser
    LLVMPowerPCDesc
    LLVMPowerPCInfo
    LLVMPowerPCAsmPrinter
    LLVMSparcDisassembler
    LLVMSparcCodeGen
    LLVMSparcAsmParser
    LLVMSparcDesc
    LLVMSparcInfo
    LLVMSparcAsmPrinter
    LLVMTableGen
    LLVMDebugInfo
    LLVMOption
    LLVMX86Disassembler
    LLVMX86AsmParser
    LLVMX86CodeGen
    LLVMSelectionDAG
    LLVMAsmPrinter
    LLVMX86Desc
    LLVMMCDisassembler
    LLVMX86Info
    LLVMX86AsmPrinter
    LLVMX86Utils
    LLVMMCJIT
    LLVMLineEditor
    LLVMInstrumentation
    LLVMInterpreter
    LLVMExecutionEngine
    LLVMRuntimeDyld
    LLVMCodeGen
    LLVMScalarOpts
    LLVMProfileData
    LLVMObject
    LLVMMCParser
    LLVMBitReader
    LLVMInstCombine
    LLVMTransformUtils
    LLVMipa
    LLVMAnalysis
    LLVMTarget
    LLVMMC
    LLVMCore
    LLVMSupport
)

set_target_properties(smokegen PROPERTIES COMPILE_DEFINITIONS __GENERATOR_BUILDING ENABLE_EXPORTS TRUE)
target_compile_features(smokegen PRIVATE cxx_nullptr)
set_property(
    SOURCE
        astconsumer.cpp
        frontendaction.cpp
        main.cpp
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
