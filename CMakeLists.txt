find_package(Qt4 REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(${QT_INCLUDES} parser)

add_subdirectory(parser)

set(generator_SRC
    main.cpp
    generatorenvironment.cpp
    generatorvisitor.cpp
    generatorpreprocessor.cpp
    options.cpp
    type_compiler.cpp
    name_compiler.cpp
)

set(types_SRC
    type.cpp)

add_library(types SHARED ${types_SRC})
target_link_libraries(types ${QT_QTCORE_LIBRARY})

add_executable(generator ${generator_SRC})
target_link_libraries(generator ${QT_QTCORE_LIBRARY} ${QT_QTXML_LIBRARY} cppparser types)

add_subdirectory(generators)
add_subdirectory(examples)
