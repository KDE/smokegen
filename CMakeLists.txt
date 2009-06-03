find_package(Qt4 REQUIRED)

include_directories(${QT_INCLUDES} parser)

add_subdirectory(parser)

set(smokegenerator_SRC
    main.cpp
    generatorenvironment.cpp
    generatorvisitor.cpp
    generatorpreprocessor.cpp
    type_compiler.cpp
    name_compiler.cpp
    type.cpp
)

add_executable(smokegenerator ${smokegenerator_SRC})
target_link_libraries(smokegenerator ${QT_QTCORE_LIBRARY} cppparser)
