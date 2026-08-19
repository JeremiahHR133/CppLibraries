project(Converter)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PRIVATE
        source/Converter.cpp

    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/Converter/Converter.h
            include/Converter/ConverterExport.h
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        CONVERTER_LIB
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

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Lib")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
