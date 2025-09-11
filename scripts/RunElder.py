from scripts.AbstractExperiments import ExperimentRunner

seed = 1234
configs = [
    {"name": "Standard", "theories":["0", "Autonomy", "0", "Health"]},
]
er = ExperimentRunner("Elder", configs)
er.buildEnvironments()
#er.run()
#er.saveResults()


# er.plotTimeResults()
# er.plotPercentTimeResults()