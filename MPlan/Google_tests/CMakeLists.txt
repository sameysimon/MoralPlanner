project(Google_tests)
#add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# adding the Google_Tests_run target
add_executable(Google_Tests_run
        JsonMDPTest.cpp
        CompareQValues.cpp
        MO_Test.cpp
        ExtractHistories.cpp
        MEHRTests.cpp
        TestBase.hpp)

# linking Google_Tests_run with DateConverter_lib which will be tested
target_link_libraries(Google_Tests_run PRIVATE gtest gtest_main MEHRPlan_lib)

include(GoogleTest)
gtest_discover_tests(Google_Tests_run)

target_link_libraries(MPlan PUBLIC MEHRPlan_lib PRIVATE nlohmann_json::nlohmann_json)

target_compile_definitions(Google_Tests_run PRIVATE DATA_FOLDER_PATH="${CMAKE_SOURCE_DIR}/Data")