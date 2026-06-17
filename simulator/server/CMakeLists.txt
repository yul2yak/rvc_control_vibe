include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

add_executable(rvc_simulator
    main.cpp
    simulator_service.cpp
    scenario_runner.cpp
    scenario_definitions.cpp
    switchable_turn_strategy.cpp
)
target_include_directories(rvc_simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(rvc_simulator PRIVATE
    RVC_WEB_ROOT="${CMAKE_SOURCE_DIR}/simulator/web"
)
target_link_libraries(rvc_simulator PRIVATE
    rvc_adapters
    httplib::httplib
)
