from scripts.AbstractExperiments import ExperimentRunner
from copy import deepcopy

defaultConfig = {"Name": "HalCarlaEqual", "theories":["0", "utility", "0", "law"], "Budget": 6, "Horizon": 8}

halTheory={"Name":"Hal", "Type":"Utility", "Rank":0}

theoriesConfigs = {
    "HalOnly": {"Theories": [["Hal", "Utility", 0]], "Considerations": [["HalLife", "Hal"]]},
    "CarlaOnly": {"Theories": [["Carla", "Utility", 0]], "Considerations": [["CarlaLife", "Carla"]]},
    
    "Hal=Carla": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"]]},
    "Hal>Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 0]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"]]},
    "Hal<Carla": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 1]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"]]},

    "Hal=Carla=Steal": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["Law", "Absolutism", 0]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["ToSteal", "Law"]]},
    "Hal=Carla>Steal": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["Law", "Absolutism", 1]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["ToSteal", "Law"]]},
    "Hal=Carla<Steal": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 0]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["ToSteal", "Law"]]},

    "Hal=Carla>StealComp": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["Law", "Absolutism", 1]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["StealWithComp", "Law"]]},
    "Hal=Carla<StealComp": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 0]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["StealWithComp", "Law"]]},

    "Cost+Carla;H8_B6": {"Theories": [["Carla", "Utility", 0]], "Considerations": [["CarlaLife", "Carla"], ["Cost", ""]], "Horizon":8, "Budget":6},
    "Cost+Carla=Steal;H8_B6": {"Theories": [["Carla", "Utility", 0], ["Law","Absolutism",0]], "Considerations": [["CarlaLife", "Carla"], ["ToSteal", "Law"], ["Cost", ""]], "Horizon":8, "Budget":6},

    "Cost+Carla;H8_B7": {"Theories": [["Carla", "Utility", 0]], "Considerations": [["CarlaLife", "Carla"], ["Cost", ""]], "Horizon":8, "Budget":7},
    "Cost+Carla=Steal;H8_B7": {"Theories": [["Carla", "Utility", 0], ["Law","Absolutism",0]], "Considerations": [["CarlaLife", "Carla"], ["ToSteal", "Law"], ["Cost", ""]], "Horizon":8, "Budget":7},

    "Cost+Carla;H8_B5": {"Theories": [["Carla", "Utility", 0]], "Considerations": [["CarlaLife", "Carla"], ["Cost", ""]], "Horizon":8, "Budget":5},
    "Cost+Carla=Steal;H8_B5": {"Theories": [["Carla", "Utility", 0], ["Law","Absolutism",0]], "Considerations": [["CarlaLife", "Carla"], ["ToSteal", "Law"], ["Cost", ""]], "Horizon":8, "Budget":5},
}

configs = []
for name, dat in theoriesConfigs.items():
    c = deepcopy(defaultConfig)
    c["Name"] = name
    c["Theories"] = dat["Theories"]
    c["Considerations"] = dat["Considerations"]
    c["Budget"] = dat["Budget"] if "Budget" in dat.keys() else c["Budget"]
    c["Horizon"] = dat["Horizon"] if "Horizon" in dat.keys() else c["Horizon"]
    configs.append(c)

er = ExperimentRunner("LostInsulin", configs)
er.run(configRepetitions=1, envRepetitions=1)
er.saveResults()


