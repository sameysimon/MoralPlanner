# The Machine Ethics Hypothetical Retrospection (MEHR) Planner.

This is a hybrid Python and C++ project designed to solve ethical decision-making problems in various Multi-Moral Markov Decision Processes (MMMDPs) and Multi-Moral Stochastic Shortest Path Problems (MMSSPs). It integrates Multi-Objective iLAO* planning algorithm and moral theories including Utilitarianism and Deontology.

To Test experiments, use the run_experiments.py script.
It will (re-)generate environment files for the Lost Insulin domain under a number of configurations. It will call the planner on each configuration 5 times. Results are stored in the Data/Experiments directory. CSV files are generated to summarise the results in the same folder. You will need the Pandas and NumPy Python packages for this to work.

Multi-Moral MDP environments are given to the planner in a JSON format. To create JSON MDP files, we use the EnvironmentBuilder Python package. It is tested with Python 3.12.2, though most versions should work fine. Only the argparse package is required for environment generation.

The Planner is implemented in C++ 17. A CMakeLists file is included and we only use the nlohmann JSON library for importing the environments. For testing, we also use the Google Tests library which is included in this repo.

---

## Features

- **Customizable MDP Environments**:
  - Define unique environments with Python Modules.
  - Includes pre-built scenarios like `LostInsulin`, `LostInsulin` (under-construction), and (under-construction) `TutorProblem`. 

- **Hybrid Python and C++ Implementation**:
  - Python modules for environment setup.
  - C++ modules for efficient planning and solution extraction.

- **Ethical Theories and Decision Making**:
  - Implements various moral theories (e.g., Utilitarianism, Absolutism Deontology).

- **Testing Framework**:
  - Includes Google Test-based unit tests for C++ components.

---

## Getting Started

### Prerequisites

- **C++ Compiler**:
  - Supports C++17 or later. Tested on Clang.
- **CMake**:
  - Version 3.14 or higher.
- **Python**:
  - Version 3.8 or higher.
- **Libraries**:
  - [`nlohmann_json`](https://github.com/nlohmann/json) for JSON processing.

### Installation

#### Clone the Repository

```bash
git clone https://github.com/your-username/MoralPlanner.git
cd MoralPlanner
```

#### Build the Project

1. Create a build directory and run CMake:

   ```bash
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
cd ../EnvironmentBuilder
pip install -r requirements.txt
```

---

## Project Structure

```
MoralPlanner
├── Data/                   # Data storage for experiments, outputs, and MDPs
├── EnvironmentBuilder/     # Python scripts for MDP environment generation
├── MPlan/                  # Core C++ library and executable
│   ├── MEHRPlan_lib/       # Library for planners and moral theories
│   ├── Google_tests/       # Unit tests for C++ components
├── run_experiments.py      # Orchestrates experiments and evaluations
```

---

## Usage

### Running the Python Environment Builder

Set up or modify MDP environments:

```bash
cd EnvironmentBuilder
python -m EnvironmentBuilder --config config.json
```

### Running the Planner

Execute the planner:

```bash
cd build
./MPlan
```

### Running Unit Tests

Run the included tests:

```bash
cd build
ctest
```

---

## Contributing

Contributions are welcome! Please fork the repository and submit a pull request. Before submitting changes, ensure that:

1. The code passes all unit tests.
2. New features are documented.


---

## Authors

- **Simon Kolker** – (https://github.com/sameysimon)

For questions or suggestions, feel free to reach out via [simon.kolker@manchester.ac.uk](mailto:simon.kolker@manchester.ac.uk).
```
