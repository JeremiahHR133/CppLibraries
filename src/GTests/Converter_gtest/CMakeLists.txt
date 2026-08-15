enable_testing()

project(Converter_gtest)

add_executable(${PROJECT_NAME}
    converter_gtest.cpp
)

target_link_libraries(${PROJECT_NAME}
    GTest::gtest_main
    Converter
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Test")

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
