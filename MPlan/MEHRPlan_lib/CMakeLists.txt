project(MEHRPlan_lib)

include_directories(Environment, Planner, Environment/MoralTheories)

set(HEADER_FILES
        Environment/Successor.hpp
        Environment/Action.hpp
        Environment/State.hpp
        Planner/ExtractHistories.hpp
        Environment/MoralTheories/MoralTheory.hpp
        Environment/MDP.hpp
        Planner/Solver.hpp
        Planner/QValue.hpp
        Environment/MoralTheories/Utilitarianism.hpp
        Environment/MoralTheories/DoubleEffect.hpp
        Environment/MoralTheories/Absolutism.hpp
        Environment/MoralTheories/Threshold.hpp
        Environment/MoralTheories/Absolutism.hpp
        Planner/ExtractSolutions.hpp
        Planner/MEHR.hpp
        Logger.hpp
        Environment/MoralTheories/Maximin.hpp
)

set(SOURCE_FILES
        Environment/MoralTheories/Utilitarianism.cpp
        Environment/MoralTheories/Threshold.cpp
        Environment/Action.cpp
        Environment/MDP.cpp
        Environment/State.cpp
        Environment/Successor.cpp
        Planner/QValue.cpp
        Planner/Solver.cpp
        Environment/MDPBuilder.cpp
        Planner/MO_Solver.cpp
        Planner/MEHR.cpp
        Planner/AttackType.hpp
        Planner/MO_ILAO_STAR.cpp
        Environment/MoralTheories/Absolutism.cpp
        Planner/Policy.hpp
        Planner/PostOrderDFS.cpp
        Planner/ExtractHistories.cpp
        Environment/MoralTheories/DoubleEffect.cpp
        Environment/MoralTheories/Maximin.cpp
        Environment/MoralTheories/MoralTheory.cpp
        Planner/BuildHeuristic.cpp
)


add_library(MEHRPlan_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_compile_definitions(MEHRPlan_lib PRIVATE TIME_METRIC_STR=milliseconds)

set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/_deps/nlohmann_json-build" ${CMAKE_PREFIX_PATH})
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(MEHRPlan_lib nlohmann_json::nlohmann_json)

# Enable debug definitions if building in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()
