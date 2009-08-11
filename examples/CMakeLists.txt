add_subdirectory(qt)
add_subdirectory(qtuitools)

find_package(KDE4)

if (KDE4_FOUND)
    add_subdirectory(kde)
    add_subdirectory(khtml)
endif (KDE4_FOUND)
