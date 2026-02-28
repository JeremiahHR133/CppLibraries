add_subdirectory(Logger)
add_subdirectory(Converter)
add_subdirectory(Meta)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_subdirectory(Tests)
    add_subdirectory(GTests)
endif()