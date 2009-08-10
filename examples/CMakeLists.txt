add_subdirectory(qt)

find_package(KDE4)

if (KDE4_FOUND)
    add_subdirectory(kde)
    add_subdirectory(khtml)
endif (KDE4_FOUND)
