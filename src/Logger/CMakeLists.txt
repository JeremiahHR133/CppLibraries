project(Logger)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PRIVATE
        source/Logger.cpp

    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/Logger/Logger.h
            include/Logger/LoggerExport.h
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        LOGGER_LIB
)


install(
    TARGETS ${PROJECT_NAME}
    EXPORT CppLibraries
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Lib")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
