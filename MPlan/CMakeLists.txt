cmake_minimum_required(VERSION 3.26)
project(MPlan)

set(CMAKE_CXX_STANDARD 14)

#include_directories(include)
#include_directories(include/Expected)
include_directories(MEHRPlan_lib/Environment)
include_directories(MEHRPlan_lib/Environment/MoralTheories)
include_directories(MEHRPlan_lib/Planner)



add_executable(MPlan main.cpp)
find_package(nlohmann_json CONFIG REQUIRED)

include_directories(MEHRPlan_lib)
add_subdirectory(MEHRPlan_lib)
add_subdirectory(Google_tests)

target_link_libraries(MPlan PUBLIC MEHRPlan_lib PRIVATE nlohmann_json::nlohmann_json)

target_compile_definitions(MPlan PRIVATE DATA_FOLDER_PATH="${CMAKE_SOURCE_DIR}/Data")