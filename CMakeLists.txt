cmake_minimum_required(VERSION 3.16)
project(rvc_controller VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

include(GNUInstallDirs)

add_library(rvc_domain STATIC
    src/domain/cleaning_state_machine.cpp
)
target_include_directories(rvc_domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(rvc_ports INTERFACE)
target_include_directories(rvc_ports INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(rvc_application STATIC
    src/application/cleaning_service.cpp
)
target_link_libraries(rvc_application PUBLIC rvc_domain rvc_ports)

add_library(rvc_adapters STATIC
    src/adapters/in_memory/in_memory_sensor_reader.cpp
    src/adapters/in_memory/in_memory_motion_actuator.cpp
    src/adapters/in_memory/in_memory_cleaning_actuator.cpp
    src/adapters/in_memory/fake_timer.cpp
    src/adapters/in_memory/prefer_left_turn_strategy.cpp
    src/adapters/in_memory/fixed_turn_strategy.cpp
    src/adapters/in_memory/cleaning_session_adapter.cpp
)
target_link_libraries(rvc_adapters PUBLIC rvc_application)

add_executable(rvc_cli
    src/adapters/cli/main.cpp
)
target_link_libraries(rvc_cli PRIVATE rvc_adapters)

option(RVC_BUILD_SIMULATOR "Build web simulator server" ON)
if(RVC_BUILD_SIMULATOR)
    add_subdirectory(simulator/server)
endif()

option(RVC_BUILD_TESTS "Build unit tests" ON)
if(RVC_BUILD_TESTS)
    enable_testing()
    include(cmake/gtest.cmake)

    add_executable(rvc_tests
        tests/domain/cleaning_state_machine_test.cpp
        tests/application/cleaning_service_test.cpp
        tests/application/test_fakes.cpp
    )
    target_include_directories(rvc_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(rvc_tests PRIVATE
        rvc_application
        rvc_adapters
        GTest::gtest_main
    )
    include(GoogleTest)
    gtest_discover_tests(rvc_tests)
endif()
