# Host-side unit tests (Google Test) for STM32 motor-controller headers/libs.
#
# Build (WSL/Linux):
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build
#   ctest --test-dir build --output-on-failure

cmake_minimum_required(VERSION 3.16)

project(stm32_gtest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

set(MOTOR_CONTROLLER_INC "${CMAKE_CURRENT_SOURCE_DIR}/../motor-controller/Inc")

add_executable(smoke_test tests/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE GTest::gtest_main)
gtest_discover_tests(smoke_test)

add_executable(motor_controller_tests tests/ring_buffer_test.cpp)
target_include_directories(motor_controller_tests PRIVATE "${MOTOR_CONTROLLER_INC}")
target_link_libraries(motor_controller_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(motor_controller_tests)
