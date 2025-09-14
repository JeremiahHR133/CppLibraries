project(Tests)

set(SOURCES
    ./main.cpp
)

add_executable(${PROJECT_NAME}
	${SOURCES}
	${HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Logger
    Converter
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "App")

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
)