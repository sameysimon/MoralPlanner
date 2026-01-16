from scripts.AbstractExperiments import ExperimentRunner
from copy import deepcopy
seed = 12345

otherConfigs = [
    {"BranchF": 2, "ActionF": 2, "GoalP": 0, "Budget": 0, "Horizon": 4, "Seed": seed},
    {"BranchF": 2, "ActionF": 2, "GoalP": 0, "Budget": 0, "Horizon": 5, "Seed": seed},
    {"BranchF": 2, "ActionF": 2, "GoalP": 0, "Budget": 0, "Horizon": 6, "Seed": seed},
    {"BranchF": 2, "ActionF": 2, "GoalP": 0, "Budget": 0, "Horizon": 7, "Seed": seed}
]

theoriesConfigs = {
    "UtilLaw": {"Theories": [["util_theory", "Utility", 0], ["law", "Absolutism", 0]],
                          "Considerations": [["utility", "util_theory"], ["law", "law"]]},
}
"""
theoriesConfigs = {
    "UtilUtil": {"Theories": [["util_theory", "Utility", 0], ["util_theory_2", "Utility", 0], ["law", "Absolutism", 0]],
                          "Considerations": [["utility1", "util_theory"], ["utility2", "util_theory_2"], ["law", "law"]]}
}
"""
configs = []
for name, t_con in theoriesConfigs.items():
    for i in range(len(otherConfigs)):
        c = deepcopy(t_con)
        c.update(otherConfigs[i])
        c["Name"] = f"{name}{i}"
        configs.append(c)


er = ExperimentRunner('Random', configs)
er.run(configRepetitions=3, envRepetitions=3)
er.saveResults()
er.plotTimeResults()
er.plotPercentTimeResults()


# "theories": [{"name":"utility", "type": "utility", "rank":0}]
# "considerations": ["name":"u1", "type":"utility", "componentOf":["utility"]]
#or
#"theories": [["utility", "utility", 0]]
#"considerations": [["u1", "utility", ["utility"]]]
