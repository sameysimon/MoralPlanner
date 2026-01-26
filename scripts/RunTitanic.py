from scripts.AbstractExperiments import ExperimentRunner
from copy import deepcopy
import subprocess
import os



explain_plan = "/"
defaultConfig = {"Name": "", "Budget": 6, "Horizon": 8}

# halTheory={"Name":"Hal", "Type":"Utility", "Rank":0
theoriesConfigs = {
    "1=3=C": {"Theories": [["First", "Utility", 0], ["Third", "Utility", 0], ["Crew", "Utility", 0]], "Considerations": [["1", "First"], ["3", "Third"], ["crew", "Crew"]]},
    #"First": {"Theories": [["Utility", "Utility", 0]], "Considerations": [["1", "Utility"]]},
    #"Third": {"Theories": [["Utility", "Utility", 0]], "Considerations": [["3", "Utility"]]},
    #"Crew": {"Theories": [["Utility", "Utility", 0]], "Considerations": [["crew", "Utility"]]},

}

configs = []
for name, dat in theoriesConfigs.items():
    c = deepcopy(defaultConfig)
    c["Name"] = name
    c["Theories"] = dat["Theories"]
    c["Considerations"] = dat["Considerations"]
    configs.append(c)

er = ExperimentRunner("Titanic", configs)
er.run(configRepetitions=1, envRepetitions=1)
result = subprocess.run([])

