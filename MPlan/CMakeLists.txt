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
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL REQUIRED)

include_directories(MEHRPlan_lib)
add_subdirectory(MEHRPlan_lib)
add_subdirectory(Google_tests)

target_link_libraries(MPlan PUBLIC MEHRPlan_lib PRIVATE nlohmann_json::nlohmann_json CURL::libcurl)

target_compile_definitions(MPlan PRIVATE DATA_FOLDER_PATH="${CMAKE_SOURCE_DIR}/../Data/MDPs/")
target_compile_definitions(MPlan PRIVATE OUTPUT_FOLDER_PATH="${CMAKE_SOURCE_DIR}/../Data/MPlan-Out/")