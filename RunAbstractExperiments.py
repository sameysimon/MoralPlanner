from AbstractExperiments import ExperimentRunner
import matplotlib as plt
from datetime import datetime

import pandas as pd
import os
seed = 1234
configs = [
    {"name": "0Util_0Law__hor=2", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 2, "seed": seed},
    {"name": "0Util_0Law__hor=3", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 3, "seed": seed},
    {"name": "0Util_0Law__hor=4", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 4, "seed": seed},
    {"name": "0Util_0Law__hor=5", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 5, "seed": seed},
    {"name": "0Util_0Law__hor=6", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 6, "seed": seed},
    {"name": "0Util_0Law__hor=7", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 7, "seed": seed}
]
outf = os.getcwd() + "/MoralPlanner/Data/Experiments/Random/" + datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "/"
er = ExperimentRunner(outFolder=outf)
er.run_random_configs(domain='random', configs=configs, configRepetitions=4, envRepetitions=3)
er.saveResults()
er.plotTimeResults()
er.plotPercentTimeResults()


