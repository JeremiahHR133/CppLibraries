project(Converter)

set(SOURCES
	./source/Converter.cpp
)

set(HEADERS
	./include/Converter/Converter.h
	./include/Converter/ConverterExport.h
)

add_library(${PROJECT_NAME} SHARED
	${SOURCES}
	${HEADERS}
)

target_include_directories(${PROJECT_NAME} PUBLIC
	include
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Logger
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    CONVERTER_LIB
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Lib")

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
)