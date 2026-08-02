from scripts.AbstractExperiments import ExperimentRunner
from EnvironmentBuilder.MDPFactory import Domains
from copy import deepcopy


configs = [
    {
        "Name": "Hal=Carla", "Horizon": 2,
        "Theories": [["Act-Utilitarianism", "Utility", 0], ["Altruism for Carla", "Utility", 1], ["Egoism from Hal", "Utility", 1], ['No Stealing', 'Absolutism', 0]],
        "Considerations": [["CarlaLife", ["Altruism for Carla", "Act-Utilitarianism"]], ["HalLife", ["Egoism from Hal", "Act-Utilitarianism"]], ['ToSteal', 'No Stealing']]
    },
    {
        "Name": "Hal", "Horizon": 2,
        "Theories": [["Egoism from Hal", "Utility", 1]],
        "Considerations": [["HalLife", "Egoism from Hal"]]
    },
    {
        "Name": "Cost+Carla", "Horizon": 2, "Budget": 2,
        "Theories": [["Act-Utilitarianism", "Utility", 0], ["Egoism from Hal", "Utility", 1]],
        "Considerations": [["CarlaLife", ["Altruism for Carla", "Act-Utilitarianism"]], ["HalLife", ["Egoism from Hal", "Act-Utilitarianism"]], ['ToSteal', 'No Stealing']]
    }
]

er = ExperimentRunner(Domains.BasicLostInsulin, configs)
er.buildEnvironments()
#er.run(configRepetitions=1, envRepetitions=1)
# er.saveResults()
er.StartServerAndPost(er.makeMdpFileName(configs[0]["Name"], 0))
#er.PostMDPToServer(er.makeMdpFileName(configs[0]["Name"], 0))
input("Enter to exit...")