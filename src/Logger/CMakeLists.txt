project(Logger)

set(SOURCES
	./source/Logger.cpp
)

set(HEADERS
	./include/Logger/Logger.h
	./include/Logger/LoggerExport.h
)

add_library(${PROJECT_NAME} SHARED
	${SOURCES}
	${HEADERS}
)

target_include_directories(${PROJECT_NAME} PUBLIC
	include
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    LOGGER_LIB
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Lib")

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
)