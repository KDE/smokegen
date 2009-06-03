project(parser)

add_subdirectory(tests)

########### next target ###############

set(cppparser_LIB_SRCS
    ast.cpp
    default_visitor.cpp
    memorypool.cpp
    visitor.cpp
    control.cpp
    lexer.cpp
    parser.cpp
    listnode.cpp
    tokens.cpp
    dumptree.cpp
    parsesession.cpp
    commentparser.cpp
    commentformatter.cpp
    codegenerator.cpp
    indexedstring.cpp
    stringhelpers.cpp
    
    rpp/pp-stream.cpp
    rpp/pp-macro-expander.cpp
    rpp/pp-scanner.cpp
    rpp/pp-macro.cpp
    rpp/pp-engine.cpp
    rpp/pp-internal.cpp
    rpp/pp-environment.cpp
    rpp/pp-location.cpp
    rpp/preprocessor.cpp
    rpp/chartools.cpp
)

# Note: This library doesn't follow API/ABI/BC rules and shouldn't have a SOVERSION
#       Its only purpose is to support the plugin without needing to add all source files
#       to the plugin target
add_library(cppparser SHARED ${cppparser_LIB_SRCS})
target_link_libraries(cppparser ${QT_QTCORE_LIBRARY})
