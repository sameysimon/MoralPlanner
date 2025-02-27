cmake_minimum_required(VERSION 3.26)
project(MPlan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug definitions if building in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

include_directories(MEHRPlan_lib/Environment)
include_directories(MEHRPlan_lib/Environment/MoralTheories)
include_directories(MEHRPlan_lib/Planner)


add_executable(MPlan main.cpp)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)


include_directories(MEHRPlan_lib
add_subdirectory(MEHRPlan_lib)


target_link_libraries(MPlan PUBLIC MEHRPlan_lib PRIVATE nlohmann_json::nlohmann_json)

target_compile_definitions(MPlan PRIVATE DATA_FOLDER_PATH="${CMAKE_SOURCE_DIR}/../Data/MDPs/")
target_compile_definitions(MPlan PRIVATE OUTPUT_FOLDER_PATH="${CMAKE_SOURCE_DIR}/../Data/MPlan-Out/")

enable_testing()
add_subdirectory(Google_tests)
