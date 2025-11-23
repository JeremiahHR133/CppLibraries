enable_testing()

project(Meta_gtest)

add_executable(${PROJECT_NAME}
	meta_gtest.cpp
)

target_link_libraries(${PROJECT_NAME}
	GTest::gtest_main
	Meta
	Converter
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Test")

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})