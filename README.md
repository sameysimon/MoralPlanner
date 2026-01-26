# The Machine Ethics Hypothetical Retrospection (MEHR) Planner.

This is a hybrid Python and C++ project for generating and solving ethical decision-making problems in various Multi-Moral Markov Decision Processes (MMMDPs) and Multi-Moral Stochastic Shortest Path Problems (MMSSPs). It integrates Multi-Objective AO* planning algorithm and moral theories including Utilitarianism and Deontology.

MMMDP environments are given to the planner in a JSON format. To create JSON MDP files, use the `EnvironmentBuilder` Python package. It is tested with Python 3.12.2, though most versions should work fine. Only the *argparse* package is required for environment generation.


**To execute the Lost Insulin experiments featured in our AAMAS 2025 paper**, use the `RunInsulinExperiments.py` script. It contains a list of dicts `configs` that allow you to modify experiment parameters (moral theories, rankings, horizon etc.)
It will (re-)generate environment files for each configuration and call the planner 5 times. Results are stored in the `Data/Experiments/LostInsulin` directory. Results are summarised in `experiments_out.csv`, `theoryResults.csv` and `theoryTimes.csv` in the same folder. You will need the [`Pandas`](https://pandas.pydata.org/) and [`NumPy`](https://numpy.org/) Python packages for this to work.


The Planner is implemented in C++ 17. A `CMakeLists.txt` file is included and we use the [`nlohmann_json`](https://github.com/nlohmann/json) library for importing JSON environments. For testing, we also use the [`Google Tests`](https://github.com/google/googletest) library.

---

## Getting Started

### Prerequisites

- **C++ Compiler**:
  - Supports C++17 or later. Tested on Clang.
- **CMake**:
  - Tested on version 3.14.
- **Python**:
  - Tested on version 3.8.


### Installation

#### Clone the Repository

```bash
git clone https://github.com/sameysimon/MoralPlanner.git
cd MoralPlanner
```

#### Build the Project

1. Create a build directory and run CMake:

   ```bash
   cd MPlan
   mkdir build
   cd build
   cmake ..
   ```

2. Compile the project:

   ```bash
   cmake --build .
   ```

3. Run tests (optional):

   ```bash
   ctest
   ```

#### Install Python Dependencies

Navigate to the `EnvironmentBuilder` directory and install Python dependencies:

```bash
cd EnvironmentBuilder
pip install -r requirements.txt
```
You may need to make a virtual environment: `python3 -m venv myVEnvName` then `source myVenvName/bin/activate`.

---

## Project Structure

```
MoralPlanner
├── Data/                   # Data storage for experiments, outputs, and MDPs
├── EnvironmentBuilder/     # Python scripts for MDP environment generation
├── scripts/                # Python scripts to run domain experiments
├── MPlan/                  # Core C++ library and executable
│   ├── MEHRPlan_lib/       # Library for planners and moral theories
│   ├── Google_tests/       # Unit tests for C++ planner


```

---

## Usage

### Generating random Python Environments

Generate random abstract environments for testing
```bash
cd MoralPlanner
python3 -m EnvironmentBuilder.MDPFactory --branchFactor <int> --actionFactor <int> --horizon <int> --theories <list> [options] -- <output_file>
```
This table sums it up!

| **Argument**       | **Type**    | **Required** | **Description**                                                                                           |
|---------------------|-------------|--------------|-----------------------------------------------------------------------------------------------------------|
| `--branchFactor -bf`| `int`       | Yes          | The branching factor of the MDP. Must be greater than 0.                                                  |
| `--actionFactor -a` | `int`       | Yes          | The action factor of the MDP. Must be greater than 0.                                                     |
| `--horizon -h`      | `int`       | Yes          | The horizon of the MDP. Must be greater than 1.                                                           |
| `--theories -t`     | `list`      | Yes          | A list of theories with integer-string pairs (e.g., `0 utility 1 law`).                                   |
| `--budget -b`       | `int`       | No           | Budget for the MDP. Defaults to `0`. Must be greater than 1 if provided. '--' terminates the list         |
| `--goals -g`        | `float`     | No           | The probability that states with depth > `horizon/2` are goals. Defaults to `0` (no goals).               |
| `--seed -s`         | `float`     | No           | The seed for random number generation. Defaults to `123`.                                                 |
| `--help -h`         |             | No           | For more information.                                                                                     |
| `<output_file>`     | `str`       | Yes          | Path to the output file (e.g., `outputs/my_mdp.json`). If not provided, defaults to a predefined location. |

*Note*, the number of states is exponential in time: $$|S| = \sum\limits_{t=0}^{T} (|A| \cdot B)^{t},$$ where $S$ is the set of all states, $T$ is the horizon, $|A|$ is the number of actions applicable at every state and $B$ is the number of successor states for every action.

### Running the Planner

Execute the planner:

```bash
cd MPlan/build
./MPlan ../Data/your_fun_path/exampleMMMDP.json
```

| **Argument**   | **Type** | **Required** | **Description**                                                                                      |
|----------------|----------|----------|------------------------------------------------------------------------------------------------------|
| `--Debug  -D`  | `int`    | No       | Set Debug log level. Fatal=0, Error=1, Warn=2, Info=3, Debug=4, Trace=5, All=6. Default is Warn.     |
| `<input_file>` | `string` | Yes      | Path to the environment input file. This file must be in JSON format and define the MDP structure.   |
| `<output_file>` | `string` | Yes      | Path to the output file where results will be saved. The output will include policies and MEHR data. |


### Running Experiments from the paper
Use the Python script to re-run the experiments from our paper.
```bash
python3 MoralPlanner/RunInsulinExperiments.py
```
Results will be generated in directory `MoralPlanner/Data/Experiments/<YYYY-MM-DD HH:MM:SS>`. Inside there will be all the raw output as JSON files as well as `experiments_out.csv` collecting results and `grouped_experiments_out.csv`. 

### Running Unit Tests

Run the included tests:

```bash
# Assuming MPlan is built in directory 'MPlan/build'
cd MPlan/build/
ctest
```
---
## MDP Environment JSON Fields
All MMMDP/SSPs are stored as JSON files. The following is a guide to the various keys and how information is structured.


### Main Fields
| **Key**             | **Type**                        | **Required** | **Description**                                                                                                                                                                                                             |
|---------------------|---------------------------------|--------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Horizon`           | `int`                           | Yes          | The number of state-time-state transitions until the MDP terminates.                                                                                                                                                        |
| `State_transitions` | `object[str:list] / list[list]` | Yes          | For integer index i, specifies transitions at state i as object map from action 'a' to list of transitions. Each transition is a list `[probability:float, successor state:int, ...consideration judgments]` |
| `Theories`          | `list[object]`                  | Yes          | Details below. A list of objects each specifying each moral theory.                                                                                                                                                         |
| `Considerations`    | `list[object]`                  | Yes          | Details below. A list of objects each specifying each moral consideration.                                                                                                                                                  |
| `State_time`        | `list[int]`                     | Yes          | For each state index, gives the state's time stamp.                                                                                                                                                                         |
| `Actions`           | `list[string]`                  | Yes          | The full set of actions that can be used across states.                                                                                                                                                                     |
| `State_tags`        | `list[string]`                  | Yes          | A description of each state.                                                                                                                                                                                                |
| `Goals`             | `list[int]`                     | No           | List of states indices that are goal states; for MMSSPs.                                                                                                                                                                    |

### Moral Theory Fields
Each moral theory is an object with a few required fields.

| **Key**          | **Type** | **Required** | **Description**                                                                      |
|------------------|----------|--------------|--------------------------------------------------------------------------------------|
| `Name`           | `str`    | Yes          | Domain identifier for the moral theory.                                              |
| `Type`           | `str`    | Yes          | The particular moral theory. Currently types: `Utility, Absolute, Maximin, Ordinal`. |
| `Rank`           | `int`    | Yes          | The weak-lexicographic rank states the stakeholder's preference for the theory.      |


### Moral Consideration Fields
Each moral consideration is an object with a few required fields.

| **Key**        | **Type** | **Required** | **Description**                                                                            |
|----------------|----------|--------------|--------------------------------------------------------------------------------------------|
| `Name`         | `str`    | Yes          | The number of state-time-state transitions until the MDP terminates.                       |
| `Component_of` | `str`    | Yes          | The moral theories that use this moral consideration.                                      |
| `Type`           | `str`    | Yes          | The particular type of moral consideration. Currently types: `Utility, Absolute, Ordinal`. |
| `Default`      | `any`    | No           | A default/null value. Represents morally irrelevent information.                           |
| `Heuristic`    | `list`   | No           | A domain-dependent heuristic for every state.                                              |

## Code Authors

- **Simon Kolker** – (https://simonkolker.com)

For questions or suggestions, feel free to reach out via [simon.kolker@manchester.ac.uk](mailto:simon.kolker@manchester.ac.uk).
