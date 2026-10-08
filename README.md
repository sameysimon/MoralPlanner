# The Machine Ethics Hypothetical Retrospection (MEHR) Planner.

  This project contains a heuristic solver for
  Multi-Moral Markov Decision Processes (MMMDPs) and Multi-Moral Stochastic Shortest Path Problems (MMSSPs). These formalisms are introduced in the publication [Uncertain Machine Ethics Planning](https://dl.acm.org/doi/abs/10.5555/3709347.3743636) and in the upcoming PhD thesis Ethical Planning and Decision Making under Moral and Outcome Uncertainty.

  The experimental code from the thesis is launched from Jupyter Notebook files, described below. The outputs for the AAMAS paper are in `Data/LostInsulin/AAMAS Results`.
  
  The solver adapts Multi-Objective Dynamic Programming over for the Pareto Front of policies in a  Multi-Objective Markov Decision Process.

MMMDP/MMSSPs  are specified in a JSON format, created using the `EnvironmentBuilder` Python package. 


## Installation

#### Clone the Repository

```bash
git clone https://github.com/sameysimon/MoralPlanner.git
cd MoralPlanner
```

#### Build the Planner Project

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

#### Install the Python Environment

It is recommended that you build a virtual environment for this project. First, navigate to the project root.

**Without Conda**, create a virtual environment, activate it with source, then install the packages from the `pyproject.toml` file
```bash
python3 -m venv .venv
source .venv/bin/activate [MACOS/LINUX]
.\.venv\Scripts\Activate.ps1 [WINDOWS]
python -m pip install -e .
```
Similary, **using Conda**,
```bash
conda create --name MoralPlanner python pip
conda activate MoralPlanner
python -m pip install .
```
Each time you come back in a new terminal, you will have to use either `source .venv/bin/activate` or `conda activate MoralPlanner`.


## Usage

### Running Scripts
There are some example executions in the scripts directory. They must be executed as a module from the project root, for example,
```bash
python -m scripts.RunBasicInsulin
```
If interacting with this code base in Visual Studio Code, most scripts have a launch configuration in `.vscode/launch.json`. Therefore, if you open the project from the root, scripts can be executed from the Run and Debug tab on the left. 

### Running Notebooks
The notebooks can be opened using a Jupyter Notebook local server. From the project root, call
```
python -m jupyterlab
```
Use the GUI to find the experiments in the `Notebooks` directory. Original results can be examined by executing the first 'setup' cell, then skipping the cells calling `buildEnvironments` and `runPlanner`. Executing the following cells processes the original data.

I recommend using the Jupyter extension for Visual Studio Code, rather than the standalone Jupyter local server.

### Running the Planner

To execute the planner on its own, build it as described above, then call it with a MMMDP/MMSSP .json file, as shown below. The file path does not need to be relative. 

```bash
cd MPlan/build
./MPlan ../Data/your_fun_path/exampleMMMDP.json
```
There are a few optional arguments:
| **Argument**   | **Type** | **Required** | **Description**                                                                                      |
|----------------|----------|----------|------------------------------------------------------------------------------------------------------|
| `--Debug  -D`  | `int`    | No       | Set Debug log level. Fatal=0, Error=1, Warn=2, Info=3, Debug=4, Trace=5, All=6. Default is Warn.     |
| `<input_file>` | `string` | Yes      | Path to the environment input file. This file must be in JSON format and define the MDP structure.   |
| `<output_file>` | `string` | Yes      | Path to the output file where results will be saved. The output will include policies and MEHR data. |


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
