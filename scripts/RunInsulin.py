from scripts.AbstractExperiments import ExperimentRunner
from copy import deepcopy


defaultConfig = {"name": "HalCarlaEqual", "theories":["0", "utility", "0", "law"], "budget": 6, "horizon": 8}
theoriesConfigs = {
    "HalCarlaEqual": ["0", "CarlaLife", "0", "HalLife"],
    "HalHigherCarla": ["0", "CarlaLife", "1", "HalLife"],
    "HalCarlaHigher": ["1", "CarlaLife", "0", "HalLife"],
    "HalCarlaNoSteal": ["0", "CarlaLife", "0", "HalLife", "0", "ToSteal"],
    "HalCarlaNoStealLower": ["1", "CarlaLife", "1", "HalLife", "0", "ToSteal"],
    "HalCarlaNoStealCompLower": ["1", "CarlaLife", "1", "HalLife", "0", "StealWithComp"],
    "HalCarlaNoStealHigher": ["1", "CarlaLife", "1", "HalLife", "2", "ToSteal"],
    "HalCarlaNoStealCompHigher": ["1", "CarlaLife", "1", "HalLife", "2", "StealWithComp"],
    "CostCarla": ["0", "CarlaLife", "0", "Cost"],
    "CostCarlaSteal": ["0", "CarlaLife", "0", "Cost", "0", "ToSteal"]
}

configs = []
for name, theories in theoriesConfigs.items():
    c = deepcopy(defaultConfig)
    c["name"] = name
    c["theories"] = theories
    configs.append(c)

er = ExperimentRunner("LostInsulin", configs)
er.run(configRepetitions=1, envRepetitions=5)
er.saveResults()



