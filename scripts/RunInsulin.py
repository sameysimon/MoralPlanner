from scripts.AbstractExperiments import ExperimentRunner
from scripts.TexTables import SaveDataFrameToTexTemplate
from copy import deepcopy
import pandas as pd
import numpy as np

defaultConfig = {"Name": "HalCarlaEqual", "Budget": 3, "Horizon": 3}

halTheory={"Name":"Hal", "Type":"Utility", "Rank":0}



theoriesConfigs = {

    "Hal&Carla": {"Theories": [["AU", "Utility", 0], ["Legal Necessity", "Utility", 0]], 
                  "Considerations": [["CarlaLife", "AU"],
                                     ["HalLife", "AU"],
                                     ["Necessity", "Legal Necessity"]
                                     ]
                },

    "Hal&Carla": {"Theories": [["AU", "Utility", 0]], "Considerations": [["CarlaLife", "AU"], ["HalLife", "AU"]]},
    "Hal=Carla": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"]]},
    "Fairness": {"Theories": [["Fair", "Fairness", 1]], "Considerations": [["CarlaLife", "Fair"], ["HalLife", "Fair"]]},

    "H=C>H&C": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["AU", "Utility", 1]], "Considerations": [["CarlaLife", ["AU", "Carla"]], ["HalLife", ["AU", "Hal"]]]},
    "H=C>Fair": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["Fair", "Fairness", 1]], "Considerations": [["CarlaLife", ["Fair", "Carla"]], ["HalLife", ["Fair", "Hal"]]]},
    "H=C>Fair=H&C": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["Fair", "Fairness", 1], ["AU", "Utility", 1]], "Considerations": [["CarlaLife", ["AU", "Fair", "Carla"]], ["HalLife", ["AU", "Fair", "Hal"]]]},
    
    "Fair=Hal=Carla=AU": {"Theories": [["Fair", "Fairness", 1], ["AU", "Utility", 1], ["Carla", "Utility", 1], ["Hal", "Utility", 1]], "Considerations": [["CarlaLife", ["Fair", "Hal", "AU"]], ["HalLife", ["Carla", "Fair", "AU"]]]},
    
    "Carla>Hal": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 1]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"]]},

    "Hal>Carla+Steal": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 0], ["Law", "Absolutism", 1]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["ToSteal", "Law"]]},
    "Cost,Carla=Steal;H6_B5": {"Theories": [["Carla", "Utility", 0], ["Law","Absolutism",0]], "Considerations": [["CarlaLife", "Carla"], ["ToSteal", "Law"], ["Cost", ""]]},

    "Steal>Hal=Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 0]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["ToSteal", "Law"]]},
    "StealComp>Hal=Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 0]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["StealWithComp", "Law"]]},
    "Steal=Hal=Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 1]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["ToSteal", "Law"]]},
    "StealComp=Steal=Hal=Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 1], ["Comp", "Absolutism", 1]], "Considerations": [["CarlaLife", "Carla"], ["HalLife", "Hal"], ["ToSteal", "Law"], ["StealWithComp", "Comp"]]},

    "Rawls": {"Theories": [["Rawls", "Maximin", 1]], "Considerations": [["CarlaLife", "Rawls"], ["HalLife", "Rawls"]]},
    "Fairness": {"Theories": [["Fair", "Fairness", 1]], "Considerations": [["CarlaLife", "Fair"], ["HalLife", "Fair"]]},
    "Rawls=Hal=Carla": {"Theories": [["Rawls", "Maximin", 1], ["Carla", "Utility", 1], ["Hal", "Utility", 1]], "Considerations": [["CarlaLife", ["Rawls", "Hal"]], ["HalLife", ["Carla", "Rawls"]]]},
    "Rawls=Fair=Hal=Carla": {"Theories": [["Fair", "Fairness", 1], ["Rawls", "Maximin", 1], ["Carla", "Utility", 1], ["Hal", "Utility", 1]], "Considerations": [["CarlaLife", ["Fair", "Hal"]], ["HalLife", ["Carla", "Fair"]]]},
}

theoriesConfigs = {
    "Hal&Carla": {"Theories": [["AU", "Utility", 0], ["Legal Necessity", "Ordinal", 0], ["Legal Charge", "Ordinal", 0]], 
        "Considerations": [["CarlaLife", "AU"],
                            ["HalLife", "AU"],
                            ["Necessity", "Legal Necessity"],
                            ["OrdinalLaw", "Legal Charge"]
                            ]
    },
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

# To Save envs to file...
def SaveToFile():
    er.buildEnvironments(1)

# To save envs to file then call and run experiments...
def Experiment():
    er.run(configRepetitions=1, envRepetitions=1)
    er.saveResults()
    df = pd.DataFrame(er.data)
    # Merge ToSteal with StealWithComp
    df[['ToSteal', 'StealWithComp', 'HalLife', 'Cost']] = df[['ToSteal', 'StealWithComp', 'HalLife', 'Cost']].replace("N/A", np.nan)
    df[['ToSteal']].replace('T', '\\top')
    df[['ToSteal']].replace('F', '\\bot')
    df['Steal'] = df['ToSteal'].combine_first(df['StealWithComp'])
    df['HalLife'] = df['HalLife'].combine_first(df['Cost'])
    agg_rules = {
        'HalLife': 'first',
        'CarlaLife': 'first',
        'Steal': 'first',
        'Num_of_sols': 'first',
        'Num_of_min_non_accept': 'first',
        'Min_non_accept': 'first',
        'Total_time': 'mean',
        'Total_Attacks': 'first'
    }
    summary_table = df.groupby('Config_name', sort=False).agg(agg_rules).reset_index()
    summary_table = summary_table.fillna("N/A")
    summary_table['Num_of_sols'] = summary_table.apply(lambda row: f"{int(row['Num_of_min_non_accept'])}/{int(row['Num_of_sols'])}", axis=1)
    print(summary_table)
    summary_table['Min_non_accept'] = summary_table['Min_non_accept'].round(4)
    summary_table = summary_table.drop(columns=['Config_name', 'Num_of_min_non_accept'])
    
    SaveDataFrameToTexTemplate(summary_table, f"{er.texTablesFolder}/Insulin_worth.tex", f"{er.texOutFolder}/Insulin_worth.tex", False)
    er.plotParetoGraph(list(theoriesConfigs.keys())[0], 0, "CarlaLife", "HalLife", 0)



# To save envs to file, then start server and send experiments
def StartServerAndPost():
    er.StartServerAndPost(er.makeMdpFileName(configs[0]["Name"], 0))

# To send a file to existing server...
def PostToServer():
    er.PostMDPToServer(er.makeMdpFileName(configs[0]["Name"], 0))

#Experiment()
PostToServer()
#StartServerAndPost()
input("Enter to exit...")

