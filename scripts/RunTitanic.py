from scripts.AbstractExperiments import ExperimentRunner
from copy import deepcopy
import pandas as pd
import numpy as np

import matplotlib.pyplot as plt




explain_plan = "/"
defaultConfig = {"Name": "", "Budget": 6, "Horizon": 8}

# halTheory={"Name":"Hal", "Type":"Utility", "Rank":0
theory1Class = "Priority 1st Class"
theory3Class = "Priority 3rd Class"
theoryCrew = "Priority Crew"
theoryAU = "Act-Utilitarianism"
theoryRl = "Rawlsian"
theoryFr = "Fariness"
theoryDeon = "Deontology"


theoriesConfigs = {
    "1": {"Theories": [[theory1Class, "Utility", 0]], "Considerations": [["1", theory1Class]]},
    "3": {"Theories": [[theory3Class, "Utility", 0]], "Considerations": [["3", theory3Class]]},
    "C": {"Theories": [[theoryCrew, "Utility", 0]], "Considerations": [["crew", theoryCrew]]},

    "1=3=C": {"Theories": [[theory1Class, "Utility", 0], [theory3Class, "Utility", 0], [theoryCrew, "Utility", 0]], 
              "Considerations": [["1", theory1Class], ["3", theory3Class], ["crew", theoryCrew]]
            },

    "AU(1+3+C)=D": {"Theories": [[theoryAU, "Utility", 0], [theoryDeon, "Absolutism", 0]], 
              "Considerations": [["1", theoryAU], ["3", theoryAU], ["crew", theoryAU], ["ram", theoryDeon]],
            },
    "Rl(1+3+C)=D": {"Theories": [[theoryRl, "Maximin", 0], [theoryDeon, "Absolutism", 0]], 
              "Considerations": [["1", theoryRl], ["3", theoryRl], ["crew", theoryRl], ["ram", theoryDeon]],
            },
    "Fr(1+3+C)=Deon": {"Theories": [[theoryFr, "Fairness", 0], [theoryDeon, "Absolutism", 0]], 
                "Considerations": [["1", theoryFr], ["3", theoryFr], ["crew", theoryFr], ["ram", theoryDeon]],
            },
    
    "Rl+Fr+AU(1+3+C)=D": {"Theories": [[theoryAU, "Utility", 0], [theoryRl, "Maximin", 0], [theoryFr, "Fairness", 0], [theoryDeon, "Absolutism", 0]], 
              "Considerations": [["1", [theoryFr, theoryRl, theoryAU]], ["3", [theoryFr, theoryRl, theoryAU]], ["crew", [theoryFr, theoryRl, theoryAU]], ["ram", theoryDeon]],
            },

    "C<1+3": {"Theories": [[theory1Class, "Utility", 0], [theory3Class, "Utility", 0], [theoryCrew, "Utility", 1]], 
              "Considerations": [["1", theory1Class], ["3", theory3Class], ["crew", theoryCrew]]
            },
            

    "AU(1+3+C)=Deon": {"Theories": [[theoryAU, "Utility", 0], [theoryDeon, "Absolutism", 0]], 
              "Considerations": [["1", theoryAU], ["3", theoryAU], ["crew", theoryAU], ["ram", theoryDeon]]
            },
            
    "1=3=C=Deon": {"Theories": [[theory1Class, "Utility", 0], [theory3Class, "Utility", 0], [theoryCrew, "Utility", 0], [theoryDeon, "Absolutism", 0]], 
              "Considerations": [["1", theory1Class], ["3", theory3Class], ["crew", theoryCrew], ["ram", theoryDeon]]
            },

    "Rl(1=3=C)": {"Theories": [["First", "Utility", 0], ["Third", "Utility", 0], ["Crew", "Utility", 0]], "Considerations": [["1", "First"], ["3", "Third"], ["crew", "Crew"]]},
}

configs = []
for name, dat in theoriesConfigs.items():
    c = deepcopy(defaultConfig)
    c["Name"] = name
    c["Theories"] = dat["Theories"]
    c["Considerations"] = dat["Considerations"]
    configs.append(c)

er = ExperimentRunner("Titanic", configs)

def SaveToFile():
    er.buildEnvironments(1)

# To save envs to file then call and run experiments...
def Experiment():
    er.run(configRepetitions=1, envRepetitions=1)
    er.saveResults()

    df = pd.DataFrame(er.data)
    # Merge ToSteal with StealWithComp
    cons = ['1_class_deaths', '3_class_deaths', 'crew_deaths', 'Ram']
    df[cons] = df[cons].replace("N/A", np.nan)
    df[['Ram']].replace('T', '\\top')
    df[['Ram']].replace('F', '\\bot')

    agg_rules = {
        '1_class_deaths': 'first',
        '3_class_deaths': 'first',
        'crew_deaths': 'first',
        'Ram': 'first',
        'Number of Solutions': 'first',
        'Number of Minimal Non-Acceptability Policies': 'first',
        'Minimal Non-Acceptability': 'first',
        'Total Time': 'mean'
    }
    summary_table = df.groupby('Configuration', sort=False).agg(agg_rules).reset_index()
    summary_table = summary_table.fillna("N/A")
    summary_table['Number of Solutions'] = summary_table.apply(lambda row: f"{int(row['Number of Minimal Non-Acceptability Policies'])}/{int(row['Number of Solutions'])}", axis=1)
    print(summary_table)
    summary_table = summary_table.drop(columns=['Configuration', 'Number of Minimal Non-Acceptability Policies'])
    summary_table['Minimal Non-Acceptability'] = summary_table['Minimal Non-Acceptability'].round(4)


# To save envs to file, then start server and send experiments
def StartServerAndPost():
    er.StartServerAndPost(er.getMdpFileName(configs[0]["Name"], 0))

# To send a file to existing server...
def PostToServer():
    er.PostMDPToServer(er.getMdpFileName(configs[0]["Name"], 0))


PostToServer()