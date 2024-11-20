# The Machine Ethics Hypothetical Retrospection (MEHR) Planner.

To Test experiments, use the run_me.sh script.
It will (re-)generate environment files for the Lost Insulin domain under a number of configurations. It will call the planner on each configuration. Results are stored in the Data/MPlan-out directory.

Multi-Moral MDP environments are given to the planner in a JSON format. To create JSON MDP files, we use the EnvironmentBuilder Python package. It is tested with Python 3.12.2, though most versions should work fine. Minimal packages are required.

The Planner is implemented in C++ 17. A CMakeLists file is included and we only use the nlohmann JSON library for importing the environments. For testing we also use the Google Tests library, which is included.



