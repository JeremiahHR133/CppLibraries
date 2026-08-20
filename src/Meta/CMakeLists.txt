project(Meta)

add_library(${PROJECT_NAME} SHARED)
add_library(CppLibraries::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        source/Meta.cpp

    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/Meta/Meta.h
            include/Meta/MetaExport.h
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        META_LIB
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Logger
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT CppLibraries
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS
)


if (MSVC)
    # MSVC has weird default preprocessor behavior when expanding __VA_ARGS__
    target_compile_options(${PROJECT_NAME}
        PUBLIC
        /Zc:preprocessor
    )
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Lib")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
