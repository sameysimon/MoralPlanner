from scripts.AbstractExperiments import ExperimentRunner
from scripts.TexTables import SaveDataFrameToTexTemplate
from copy import deepcopy
import pandas as pd
import numpy as np

defaultConfig = {"Name": "HalCarlaEqual", "Budget": 3, "Horizon": 5}

halTheory={"Name":"Hal", "Type":"Utility", "Rank":0}



theoriesConfigs = {
    "AU_Strict": {"Theories": [["AU", "Utility", 0]], "Considerations": [["Overall", "AU"]]},
    "Hal&Carla": {"Theories": [["AU", "Utility", 0]], "Considerations": [["Carla", "AU"], ["Carla", "AU"]]},
    "Hal=Carla": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0]], "Considerations": [["Carla", "Carla"], ["Carla", "Hal"]]},
    "Fairness": {"Theories": [["Fair", "Fairness", 1]], "Considerations": [["Carla", "Fair"], ["Carla", "Fair"]]},

    "H=C>H&C": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["AU", "Utility", 1]], "Considerations": [["Carla", ["AU", "Carla"]], ["Carla", ["AU", "Hal"]]]},
    "H=C>Fair": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["Fair", "Fairness", 1]], "Considerations": [["Carla", ["Fair", "Carla"]], ["Carla", ["Fair", "Hal"]]]},
    "H=C>Fair=H&C": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["Fair", "Fairness", 1], ["AU", "Utility", 1]], "Considerations": [["Carla", ["AU", "Fair", "Carla"]], ["Carla", ["AU", "Fair", "Hal"]]]},
    
    "Fair=Hal=Carla=AU": {"Theories": [["Fair", "Fairness", 1], ["AU", "Utility", 1], ["Carla", "Utility", 1], ["Hal", "Utility", 1]], "Considerations": [["Carla", ["Fair", "Hal", "AU"]], ["Carla", ["Carla", "Fair", "AU"]]]},
    
    "Carla>Hal": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 1]], "Considerations": [["Carla", "Carla"], ["Carla", "Hal"]]},

    "Hal>Carla+Steal": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 0], ["Law", "Absolutism", 1]], "Considerations": [["Carla", "Carla"], ["Carla", "Hal"], ["ToSteal", "Law"]]},
    "Cost,Carla=Steal;H6_B5": {"Theories": [["Carla", "Utility", 0], ["Law","Absolutism",0]], "Considerations": [["Carla", "Carla"], ["ToSteal", "Law"], ["Cost", ""]]},

    "Steal>Hal=Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 0]], "Considerations": [["Carla", "Carla"], ["Carla", "Hal"], ["ToSteal", "Law"]]},
    "StealComp>Hal=Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 0]], "Considerations": [["Carla", "Carla"], ["Carla", "Hal"], ["StealWithComp", "Law"]]},
    "Steal=Hal=Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 1]], "Considerations": [["Carla", "Carla"], ["Carla", "Hal"], ["ToSteal", "Law"]]},
    "StealComp=Steal=Hal=Carla": {"Theories": [["Carla", "Utility", 1], ["Hal", "Utility", 1], ["Law", "Absolutism", 1], ["Comp", "Absolutism", 1]], "Considerations": [["Carla", "Carla"], ["Carla", "Hal"], ["ToSteal", "Law"], ["StealWithComp", "Comp"]]},

    "Rawls": {"Theories": [["Rawls", "Maximin", 1]], "Considerations": [["Carla", "Rawls"], ["Carla", "Rawls"]]},
    "Fairness": {"Theories": [["Fair", "Fairness", 1]], "Considerations": [["Carla", "Fair"], ["Carla", "Fair"]]},
    "Rawls=Hal=Carla": {"Theories": [["Rawls", "Maximin", 1], ["Carla", "Utility", 1], ["Hal", "Utility", 1]], "Considerations": [["Carla", ["Rawls", "Hal"]], ["Carla", ["Carla", "Rawls"]]]},
    "Rawls=Fair=Hal=Carla": {"Theories": [["Fair", "Fairness", 1], ["Rawls", "Maximin", 1], ["Carla", "Utility", 1], ["Hal", "Utility", 1]], "Considerations": [["Carla", ["Fair", "Hal"]], ["Carla", ["Carla", "Fair"]]]},
}


utilConfigs = {
    "AU_Strict": {"Theories": [["AU", "Utility", 0]], "Considerations": [["Overall", "AU"]]},
    "AU": {"Theories": [["AU", "Utility", 0]], "Considerations": [["Hal", "AU"], ["Carla", "AU"]]},
    "Carla": {"Theories": [["Carla", "Utility", 0]], "Considerations": [["Carla", "Carla"]]},
    "Hal": {"Theories": [["Hal", "Utility", 0]], "Considerations": [["Hal", "Hal"]]},
    "Hal=Carla": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0]], "Considerations": [["Carla", "Carla"], ["Hal", "Hal"]]},
}

IGNOREtheoriesConfigs = {
    "Necessity&Law": {"Theories": [["AU", "Utility", 0], ["Legal Necessity", "Ordinal", 0], ["Legal Charge", "Ordinal", 0]], 
        "Considerations": [["Carla", "AU"],
                            ["Carla", "AU"],
                            ["Necessity", "Legal Necessity"],
                            ["OrdinalLaw", "Legal Charge"]
                            ]
    },
}

configs = []
for name, dat in utilConfigs.items():
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
    df[['ToSteal', 'StealWithComp', 'Carla', 'Cost']] = df[['ToSteal', 'StealWithComp', 'Carla', 'Cost']].replace("N/A", np.nan)
    df[['ToSteal']].replace('T', '\\top')
    df[['ToSteal']].replace('F', '\\bot')
    df['Steal'] = df['ToSteal'].combine_first(df['StealWithComp'])
    df['Carla'] = df['Carla'].combine_first(df['Cost'])
    agg_rules = {
        'Carla': 'first',
        'Carla': 'first',
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
    er.plotParetoGraph(list(theoriesConfigs.keys())[0], 0, "Carla", "Carla", 0)

    df["Solutions"]



# To save envs to file, then start server and send experiments
def StartServerAndPost():
    er.StartServerAndPost(er.makeMdpFileName(configs[0]["Name"], 0))

# To send a file to existing server...
def PostToServer(configIndex=0):
    er.PostMDPToServer(er.makeMdpFileName(configs[configIndex]["Name"], 0))

#Experiment()
PostToServer(1)
#StartServerAndPost()
input("Enter to exit...")
