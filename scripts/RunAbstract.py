from scripts.AbstractExperiments import ExperimentRunner

seed = 1234
configs = [
    #{"name": "0Util_0Law__hor=2", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 2, "seed": seed},
    #{"name": "0Util_0Law__hor=3", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 3, "seed": seed},
    #{"name": "0Util_0Law__hor=4", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 4, "seed": seed},
    #{"name": "0Util_0Law__hor=5", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 5, "seed": seed},
    #{"name": "0Util_0Law__hor=6", "theories":["0", "utility", "0", "law"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 6, "seed": seed},
    {"name": "0Util_0Util__hor=2", "theories":["0", "utility", "0", "utility"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 2, "seed": seed},
    {"name": "0Util_0Util__hor=3", "theories":["0", "utility", "0", "utility"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 3, "seed": seed},
    {"name": "0Util_0Util__hor=4", "theories":["0", "utility", "0", "utility"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 4, "seed": seed},
    {"name": "0Util_0Util__hor=5", "theories":["0", "utility", "0", "utility"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 5, "seed": seed},
    #{"name": "0Util_0Util__hor=6", "theories":["0", "utility", "0", "utility"], "branchF": 2, "actionF": 2, "goalP": 0, "budget": 0, "horizon": 6, "seed": seed},
    
]
er = ExperimentRunner('random', configs)
er.run(configRepetitions=1, envRepetitions=1)
er.saveResults()
er.plotTimeResults()
er.plotPercentTimeResults()


