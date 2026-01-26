from scripts.AbstractExperiments import ExperimentRunner
from copy import deepcopy

defaultConfig = {"Name": "", "Budget": 6, "Horizon": 8}

# halTheory={"Name":"Hal", "Type":"Utility", "Rank":0}
# consideration = ["Tag", "Theory_Name"|["Theory_Name", "Theory_Name_2"]]

Rawls =  {"Theories": [["Rawls", "Maximin", 0]], "Considerations": [["red:wellbeing", "Rawls"], ["blue:wellbeing", "Rawls"]]}
Fair = {"Theories": [["Fair", "Fairness", 0]], "Considerations": [["red:wellbeing", "Fair"], ["blue:wellbeing", "Fair"]]}
FindInfo = {"Theories": [["FindInfo", "Utility", 0]], "Considerations": [["FindInfo", "Utility"]]}

theoriesConfigs = {
    "NoIgnore>Explore>Red=Blue": {"Theories": [["NeverIgnore", "Absolutism", 0], ["FindInfo", "Utility", 1], ["Red_Utility", "Utility", 2], ["Blue_Utility", "Utility", 2]],
                          "Considerations": [["FindInfo", "FindInfo"], ["blue:wellbeing", "Blue_Utility"], ["red:wellbeing", "Red_Utility"], ["NeverIgnore", "NeverIgnore"]]},
}

configs = []
for name, dat in theoriesConfigs.items():
    c = deepcopy(defaultConfig)
    c["Name"] = name
    c["Theories"] = dat["Theories"]
    c["Considerations"] = dat["Considerations"]
    configs.append(c)

er = ExperimentRunner("SearchRescue", configs)
er.buildEnvironments(1)
#er.run(configRepetitions=1, envRepetitions=1)
#er.saveResults()
