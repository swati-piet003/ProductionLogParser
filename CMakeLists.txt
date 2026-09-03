cmake_minimum_required(VERSION 3.16)
project(multithreaded_log_engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(log_engine
    src/log_parser.cpp
    src/log_processing_engine.cpp
)
target_include_directories(log_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(log_engine PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(log_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(log_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(log_processor src/main.cpp)
target_link_libraries(log_processor PRIVATE log_engine)

option(LOG_ENGINE_BUILD_TESTS "Build the test suite" ON)
if(LOG_ENGINE_BUILD_TESTS)
    enable_testing()
    add_executable(log_engine_tests tests/test_main.cpp)
    target_link_libraries(log_engine_tests PRIVATE log_engine)
    add_test(NAME log_engine_tests COMMAND log_engine_tests)
endif()

install(TARGETS log_processor RUNTIME DESTINATION bin)

