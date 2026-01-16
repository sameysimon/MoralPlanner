from scripts.AbstractExperiments import ExperimentRunner
from copy import deepcopy
seed = 1234
morals = {
    "priv=auto=health": {
        "Theories":[["M_Privacy", "Absolutism", 0], ["M_Autonomy", "Utility", 0], ["M_Health", "Utility", 0]],
        "Considerations":[["privacy", "M_Privacy"], ["autonomy", "M_Autonomy"], ["health", "M_Health"], ["cost"]]
     },
     "health>priv=auto": {
        "Theories":[["M_Privacy", "Absolutism", 2], ["M_Autonomy", "Utility", 2], ["M_Health", "Utility", 0]],
        "Considerations":[["privacy", "M_Privacy"], ["autonomy", "M_Autonomy"], ["health", "M_Health"], ["cost"]]
     },
     "Ordinal_Auto": {
        "Theories":[["M_OrdAuto", "Ordinal", 0]],
        "Considerations":[["ordinal_autonomy_pessimist", "M_OrdAuto"]]
     },
}
defaultConfig = {"Name": "", "Budget": 6, "Horizon": 8}
configs = []
for m_name, m in morals.items():
    c = deepcopy(defaultConfig)
    c["Name"] = m_name
    c.update(m)
    configs.append(c)


er = ExperimentRunner("Elder", configs)
er.buildEnvironments(1)
#er.run(configRepetitions=1, envRepetitions=1) # to build environments and run experiments on release planner AND
#er.saveResults() # this to save the results


# er.plotTimeResults()
# er.plotPercentTimeResults()