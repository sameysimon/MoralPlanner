project(MEHRPlan_lib)

include_directories(Environment, Planner, Environment/MoralTheories)

set(HEADER_FILES
    Environment/Successor.hpp
    Environment/Action.hpp
    Environment/State.hpp
    Environment/MoralTheories/MoralTheory.hpp
    Planner/Expecter.hpp
    Environment/MDP.hpp
    Planner/Solver.hpp
    Planner/QValue.hpp
    Environment/MoralTheories/Utilitarianism.hpp
    Planner/Solution.hpp
    Planner/MEHR.hpp
)

set(SOURCE_FILES
        Environment/MoralTheories/Utilitarianism.cpp
        Environment/Action.cpp
        Environment/MDP.cpp
        Environment/State.cpp
        Environment/Successor.cpp
        Planner/Expecter.cpp
        Planner/QValue.cpp
        Planner/Solution.cpp
        Planner/Solver.cpp
        Environment/MoralTheories/MoralTheory.cpp
        Environment/MDPBuilder.cpp
        Planner/MO_Solver.cpp
        Planner/MEHR.cpp
        Planner/Attack.hpp
        Planner/MO_ILAO_STAR.cpp
)


add_library(MEHRPlan_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(MEHRPlan_lib nlohmann_json::nlohmann_json)
