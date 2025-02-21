from AbstractExperiments import ExperimentRunner
import matplotlib as plt
from datetime import datetime
from copy import deepcopy
import pandas as pd
import numpy as np
import os


defaultConfig = {"name": "HalCarlaEqual", "theories":["0", "utility", "0", "law"], "budget": 18.5, "horizon": 20}
configs = {
    "HalCarlaEqual": ["0", "CarlaLife", "0", "HalLife"],
    "HalHigherCarla": ["0", "CarlaLife", "1", "HalLife"],
    "HalCarlaHigher": ["1", "CarlaLife", "0", "HalLife"],
    "HalCarlaNoSteal": ["0", "CarlaLife", "0", "HalLife", "0", "ToSteal"],
    "HalCarlaNoStealComp": ["1", "CarlaLife", "0", "HalLife", "0", "StealWithComp"],
    "CostCarla": ["0", "CarlaLife", "0", "Cost"],
    "CostCarlaSteal": ["0", "CarlaLife", "0", "Cost", "0", "ToSteal"]
}

finalConfigs = []
for name, theories in configs.items():
    c = deepcopy(defaultConfig)
    c["name"] = name
    c["theories"] = theories
    finalConfigs.append(c)


outf = os.getcwd() + "/MoralPlanner/Data/Experiments/LostInsulin/" + datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "/"
er = ExperimentRunner(outFolder=outf)
er.run_random_configs(domain='LostInsulin', configs=finalConfigs, configRepetitions=1, envRepetitions=5)


# Save data in tables
df = pd.DataFrame(er.data)
df.to_csv(outf + "experiments_out.csv")

theoryTimes = df.groupby("theories", sort=False)[["total_time", "plan_time", "sol_time", "out_time", "mehr_time", "expanded_states", "iterations", "backups", "min_non_accept", "num_of_sols"]].mean()
theoryTimes.round(4)
theoryTimes.to_csv(outf + "theoryTimes.csv")

# want to make sure theory expectaions are equal across the same theory config
cols = ["theories", "min_non_accept"]
cols.extend(er.getTheoryTags(finalConfigs))
uniqueValues = df[cols].groupby('theories', sort=False).nunique()
if (not (uniqueValues==1).all().all()):
    raise Exception("Me: Different iterations returned different results!")

theoryResults = df[cols].groupby('theories', sort=False).first()
theoryResults.to_csv(outf + "theoryResults.csv")



