project(Google_tests)
#add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# adding the Google_Tests_run target
add_executable(Google_Tests_run
        test_JsonToMDP.cpp
        test_QValueCompare.cpp
        test_ExtractHistories.cpp
        test_MEHR.cpp
        TestBase.hpp
        test_maximin.cpp
        test_explain.cpp
        test_FromSourceFile.cpp
)

# linking Google_Tests_run with DateConverter_lib which will be tested
target_link_libraries(Google_Tests_run PRIVATE gtest gtest_main MEHRPlan_lib)

include(GoogleTest)
gtest_discover_tests(Google_Tests_run)

target_link_libraries(Google_Tests_run PUBLIC MEHRPlan_lib PRIVATE nlohmann_json::nlohmann_json)

target_compile_definitions(Google_Tests_run PRIVATE DATA_FOLDER_PATH="${CMAKE_SOURCE_DIR}/../Data/MDPs/")
target_compile_definitions(Google_Tests_run PRIVATE OUTPUT_FOLDER_PATH="${CMAKE_SOURCE_DIR}/../Data/MPlan-Out/")