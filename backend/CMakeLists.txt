cmake_minimum_required(VERSION 3.16)
project(crossbow_sim_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native)
    endif()
endif()

option(USE_BOOST_LOCKFREE "Use Boost.Lockfree for high-performance queues" OFF)
option(USE_SPDLOG "Use spdlog for structured logging" ON)
option(USE_PROMETHEUS "Enable Prometheus metrics endpoint" ON)
option(USE_CLICKHOUSE "Enable ClickHouse storage backend" ON)
option(USE_MQTT "Enable MQTT alert publishing" ON)

find_package(Threads REQUIRED)

set(SOURCES
    src/main.cpp
    src/dynamics_model.cpp
    src/clickhouse_storage.cpp
    src/udp_receiver.cpp
    src/accuracy_analyzer.cpp
    src/http_server.cpp
    src/ballistic_simulator.cpp
    src/accuracy_analyzer_service.cpp
    src/alarm_mqtt_service.cpp
    src/mqtt_alert_manager.cpp
    src/logger.cpp
    src/metrics.cpp
    src/bowstring_material.cpp
    src/sight_optics.cpp
    src/formation_simulator.cpp
)

set(HEADERS
    include/common.h
    include/message_queue.h
    include/config_manager.h
    include/dynamics_model.h
    include/clickhouse_storage.h
    include/udp_receiver.h
    include/accuracy_analyzer.h
    include/http_server.h
    include/ballistic_simulator.h
    include/accuracy_analyzer_service.h
    include/alarm_mqtt_service.h
    include/mqtt_alert_manager.h
    include/logger.h
    include/metrics.h
    include/bowstring_material.h
    include/sight_optics.h
    include/formation_simulator.h
)

add_executable(crossbow_backend ${SOURCES} ${HEADERS})

target_include_directories(crossbow_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# JSON library
find_package(nlohmann_json 3.11.0 QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(crossbow_backend PRIVATE nlohmann_json::nlohmann_json)
    message(STATUS "Using system nlohmann_json")
else()
    target_include_directories(crossbow_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/json/include)
    message(STATUS "Using bundled nlohmann_json (third_party)")
endif()

# Boost (optional)
if(USE_BOOST_LOCKFREE)
    find_package(Boost 1.70 REQUIRED COMPONENTS system)
    if(Boost_FOUND)
        target_compile_definitions(crossbow_backend PRIVATE HAS_BOOST_LOCKFREE)
        target_include_directories(crossbow_backend PRIVATE ${Boost_INCLUDE_DIRS})
        target_link_libraries(crossbow_backend PRIVATE ${Boost_LIBRARIES})
        message(STATUS "Boost.Lockfree enabled")
    endif()
else()
    message(STATUS "Boost.Lockfree disabled - using std::mutex based MessageQueue (default)")
endif()

# spdlog
if(USE_SPDLOG)
    find_package(spdlog 1.10 QUIET)
    if(spdlog_FOUND)
        target_link_libraries(crossbow_backend PRIVATE spdlog::spdlog)
        target_compile_definitions(crossbow_backend PRIVATE HAS_SPDLOG)
        message(STATUS "Using system spdlog")
    else()
        include(FetchContent)
        FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.12.0
        )
        FetchContent_MakeAvailable(spdlog)
        target_link_libraries(crossbow_backend PRIVATE spdlog::spdlog)
        target_compile_definitions(crossbow_backend PRIVATE HAS_SPDLOG)
        message(STATUS "Using fetched spdlog v1.12.0")
    endif()
endif()

# Prometheus
if(USE_PROMETHEUS)
    find_package(prometheus-cpp QUIET)
    if(prometheus-cpp_FOUND)
        target_link_libraries(crossbow_backend PRIVATE prometheus-cpp::core prometheus-cpp::pull)
        target_compile_definitions(crossbow_backend PRIVATE HAS_PROMETHEUS)
        message(STATUS "Using system prometheus-cpp")
    else()
        include(FetchContent)
        FetchContent_Declare(
            prometheus-cpp
            GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp.git
            GIT_TAG v1.1.0
        )
        set(ENABLE_PUSH OFF CACHE BOOL "")
        set(ENABLE_COMPRESSION OFF CACHE BOOL "")
        set(ENABLE_TESTING OFF CACHE BOOL "")
        FetchContent_MakeAvailable(prometheus-cpp)
        target_link_libraries(crossbow_backend PRIVATE prometheus-cpp::core prometheus-cpp::pull)
        target_compile_definitions(crossbow_backend PRIVATE HAS_PROMETHEUS)
        message(STATUS "Using fetched prometheus-cpp v1.1.0")
    endif()
endif()

# ClickHouse
if(USE_CLICKHOUSE)
    find_package(clickhouse-cpp QUIET)
    if(clickhouse-cpp_FOUND)
        target_link_libraries(crossbow_backend PRIVATE clickhouse-cpp::clickhouse-cpp)
        target_compile_definitions(crossbow_backend PRIVATE HAS_CLICKHOUSE)
        message(STATUS "Using system clickhouse-cpp")
    else()
        target_compile_definitions(crossbow_backend PRIVATE CLICKHOUSE_MOCK=1)
        message(STATUS "clickhouse-cpp not found - using in-memory mock (production build requires clickhouse-cpp)")
    endif()
endif()

# MQTT
if(USE_MQTT)
    find_package(PahoMqttCpp QUIET)
    if(PahoMqttCpp_FOUND)
        target_link_libraries(crossbow_backend PRIVATE PahoMqttCpp::paho-mqttpp3)
        target_compile_definitions(crossbow_backend PRIVATE HAS_MQTT)
        message(STATUS "Using system paho-mqttpp")
    else()
        target_compile_definitions(crossbow_backend PRIVATE MQTT_MOCK=1)
        message(STATUS "paho-mqttpp not found - using MQTT mock (production build requires paho-mqttpp)")
    endif()
endif()

# Platform libs
if(WIN32)
    target_link_libraries(crossbow_backend PRIVATE Ws2_32)
else()
    target_link_libraries(crossbow_backend PRIVATE dl)
endif()

target_link_libraries(crossbow_backend PRIVATE Threads::Threads)

install(TARGETS crossbow_backend RUNTIME DESTINATION bin)
install(FILES config.json DESTINATION etc/crossbow)

# ================ 单元测试 ================
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    set(TEST_SOURCES
        tests/test_main.cpp
        tests/test_bowstring.cpp
        tests/test_sight_optics.cpp
        tests/test_formation.cpp
        tests/test_shooting.cpp
        src/bowstring_material.cpp
        src/sight_optics.cpp
        src/formation_simulator.cpp
        src/dynamics_model.cpp
        src/logger.cpp
        src/metrics.cpp
    )

    set(TEST_HEADERS
        tests/test_harness.h
        include/common.h
        include/dynamics_model.h
        include/bowstring_material.h
        include/sight_optics.h
        include/formation_simulator.h
        include/logger.h
        include/metrics.h
    )

    add_executable(crossbow_backend_tests ${TEST_SOURCES} ${TEST_HEADERS})

    target_include_directories(crossbow_backend_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    if(nlohmann_json_FOUND)
        target_link_libraries(crossbow_backend_tests PRIVATE nlohmann_json::nlohmann_json)
    else()
        target_include_directories(crossbow_backend_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/json/include)
    endif()

    target_compile_definitions(crossbow_backend_tests PRIVATE
        CLICKHOUSE_MOCK=1
        MQTT_MOCK=1
    )

    target_link_libraries(crossbow_backend_tests PRIVATE Threads::Threads)

    if(WIN32)
        target_link_libraries(crossbow_backend_tests PRIVATE Ws2_32)
    else()
        target_link_libraries(crossbow_backend_tests PRIVATE dl)
    endif()

    enable_testing()
    add_test(NAME crossbow_backend_tests COMMAND crossbow_backend_tests)

    message(STATUS "Unit tests enabled - build target: crossbow_backend_tests")
endif()
