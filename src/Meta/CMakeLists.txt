project(Meta)

set(SOURCES
	./source/Meta.cpp
)

set(HEADERS
	./include/Meta/Meta.h
	./include/Meta/MetaExport.h
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
	Converter
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    META_LIB
)

if (MSVC)
	# MSVC has weird default preprocessor behavior when expanding __VA_ARGS__
	target_compile_options(${PROJECT_NAME}
		PUBLIC
		/Zc:preprocessor
	)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Lib")

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
)