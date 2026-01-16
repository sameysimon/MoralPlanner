cmake_minimum_required(VERSION 3.26)
project(MPlan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")

# Enable debug definitions if building in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

include_directories(MEHRPlan_lib/Environment)
include_directories(MEHRPlan_lib/Environment/MoralTheories)
include_directories(MEHRPlan_lib/Planner)


add_executable(MPlan main.cpp
        time_config.hpp
        Runner.hpp
        crow_all.h
        REST_App.hpp
        JSONBuilder.hpp
        JSONBuilder.cpp
        REST_App.cpp
)

# Google Test for unit tests
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
)
FetchContent_MakeAvailable(googletest)

# NLOHMANN_JSON for json read/writing
FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)


# Fetch asio for the CROW REST API
FetchContent_Declare(
        asio
        URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-33-0.tar.gz
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include")
include_directories(${ASIO_INCLUDE_DIR})


include_directories(MEHRPlan_lib)
add_subdirectory(MEHRPlan_lib)


target_link_libraries(MPlan PUBLIC MEHRPlan_lib PRIVATE nlohmann_json::nlohmann_json)

target_compile_definitions(MPlan PRIVATE DATA_FOLDER_PATH="${CMAKE_SOURCE_DIR}/../Data/MDPs/")
target_compile_definitions(MPlan PRIVATE OUTPUT_FOLDER_PATH="${CMAKE_SOURCE_DIR}/../Data/MPlan-Out/")

enable_testing()
add_subdirectory(Google_tests)
