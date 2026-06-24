from scripts.AbstractExperiments import ExperimentRunner, GenerateConfigs
from scripts.TexTables import SaveDataFrameToTexTemplate
from copy import deepcopy
import pandas as pd
import numpy as np


defaultConfig = {"Budget": 3, "Horizon": 6}
Config_repetitions=1
Environment_repetitions = 10
UtilityConfigs = {
    "(Hal)": {"Theories": [["Hal", "Utility", 0]], "Considerations": [["Hal", "Hal"]]},
    "(Carla)": {"Theories": [["Carla", "Utility", 0]], "Considerations": [["Carla", "Carla"]]},
    "(AU)": {"Theories": [["AU", "Utility", 0]], "Considerations": [["Overall", "AU"]]},
    "(Hal, Carla)": {"Theories": [["AU", "Utility", 0]], "Considerations": [["Hal", "AU"], ["Carla", "AU"]]},
    "(Hal)^0, (Carla)^0": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0]], "Considerations": [["Carla", "Carla"], ["Hal", "Hal"]]},
    "(Hal)^0, (Carla)^0, (Hal,Carla)^0": {"Theories": [["Carla", "Utility", 0], ["Hal", "Utility", 0], ["AU", "Utility", 0]], "Considerations": [["Carla",["AU", "Carla"]], ["Hal", ["AU", "Hal"]]]},
    "(HalLife)^0, (CaraLife)^0": {"Theories": [["HalLife", "Utility", 0], ["CarlaLife", "Utility", 0]],
                    "Considerations": [["CarlaLife", "CarlaLife"], ["HalLife", "HalLife"]]
    },
    "(HalLife)^0, (CaraLife)^0, (HalSmall)^1, (CaraSmall)^1": {
        "Theories": [["HalLife", "Utility", 0], ["CarlaLife", "Utility", 0], ["HalSmall", "Utility", 1], ["CarlaSmall", "Utility", 1]],
        "Considerations": [["CarlaLife", "CarlaLife"], ["HalLife", "HalLife"], ["CarlaSmall", "CarlaSmall"], ["HalSmall", "HalSmall"]]
    }
}

LegalConfigs = {
    "Hal": {
        "Theories": [["Hal", "Utility", 0]], 
        "Considerations": [["Hal", "Hal"]]
    },
    "(Law)^0, (Hal)^0": {
        "Theories": [["Hal", "Utility", 0], ["OrdinalLaw", "Ordinal", 0]], 
        "Considerations": [["Hal", "Hal"], ["OrdinalLaw", "OrdinalLaw"]]
    },
    "(Hal)^0, (Necessity)^0": {
        "Theories": [["Hal", "Utility", 0], ["Necessity", "Ordinal", 0]], 
        "Considerations": [["Hal", "Hal"], ["Necessity", "Necessity"]]
    },
    "(Law)^0, (Hal)^0, (Carla)^0": {
        "Theories": [["Hal", "Utility", 0], ["OrdinalLaw", "Ordinal", 0], ["Carla", "Utility", 0]],
        "Considerations": [["Hal", "Hal"], ["OrdinalLaw", "OrdinalLaw"], ["Carla", "Carla"]]
    },
    "(Law)^0, (Necessity)^0": {
        "Theories": [["Necessity", "Ordinal", 0], ["OrdinalLaw", "Ordinal", 0]], 
        "Considerations": [["Necessity", "Necessity"], ["OrdinalLaw", "OrdinalLaw"]]
    },
    "(Law)^0, (Necessity)^0, (Hal)^0": {
        "Theories": [["Hal", "Utility", 0], ["Necessity", "Ordinal", 0], ["OrdinalLaw", "Ordinal", 0]], 
        "Considerations": [["Hal", "Hal"], ["Necessity", "Necessity"], ["OrdinalLaw", "OrdinalLaw"]]
    },
    "(Law)^0, (Necessity)^0, (Hal)^1": {
        "Theories": [["Hal", "Utility", 1], ["Necessity", "Ordinal", 0], ["OrdinalLaw", "Ordinal", 0]], 
        "Considerations": [["Hal", "Hal"], ["Necessity", "Necessity"], ["OrdinalLaw", "OrdinalLaw"]]
    },
    "(Law)^1, (Necessity)^0, (Hal)^1": {
        "Theories": [["Hal", "Utility", 1], ["Necessity", "Ordinal", 0], ["OrdinalLaw", "Ordinal", 0]], 
        "Considerations": [["Hal", "Hal"], ["Necessity", "Necessity"], ["OrdinalLaw", "OrdinalLaw"]]
    },
    "(Law)^1, (Necessity)^0, (Hal)^2": {
        "Theories": [["Hal", "Utility", 2], ["Necessity", "Ordinal", 0], ["OrdinalLaw", "Ordinal", 1]], 
        "Considerations": [["Hal", "Hal"], ["Necessity", "Necessity"], ["OrdinalLaw", "OrdinalLaw"]]
    },
}


configs = []
er = 0

# To Save envs to file...
def SaveToFile():
    er.buildEnvironments(8)

# To save envs to file, then start server and send experiments
def StartServerAndPost(configIndex=0, port=18080):
    er.StartServerAndPost(er.makeMdpFileName(configs[configIndex]["Name"], 0), port)

# To send a file to existing server...
def PostToServer(configIndex=0, port=18080):
    er.PostMDPToServer(er.makeMdpFileName(configs[configIndex]["Name"], 0), port)

def PostAllConfigsToServer():
    port = 18080
    for i in range(len(configs)):
        print(f"Running config {configs[i]["Name"]} on port {port+i}...")
        PostToServer(i, port=port)
        input("Done. Enter to continue.")


# To save envs to file then call and run experiments...
def UtilityExperiment():
    defaultConfig = {"Budget": 3, "Horizon": 6}
    configs = GenerateConfigs(UtilityConfigs, defaultConfig)

    er = ExperimentRunner("LostInsulin", configs)
    er.buildEnvironments(configRepetitions=Config_repetitions)
    er.run(configRepetitions=Config_repetitions, envRepetitions=Environment_repetitions)
    er.saveResults()
    #
    # Utilities Summary Table
    #
    cols = ["Config_name", 'Hal_Utility', 'Carla_Utility', "Min_non_accept", "Num_of_min_non_accept", "Num_of_sols", "Total_time"]
    df = pd.DataFrame(er.data)
    df = df.replace(["", "N/A", "NA", "nan", "None"], np.nan)
    if 'Carla' in df.columns and 'CarlaLife' in df.columns:
        df['Carla_Utility'] = df['Carla'].combine_first(df['CarlaLife'])
    if 'Hal' in df.columns and 'HalLife' in df.columns:
        df['Hal_Utility'] = df['Hal'].combine_first(df['HalLife'])
    if 'Overall' in df.columns:
        df['Hal_Utility'] = df['Hal_Utility'].combine_first(df['Overall'])
    df = df.replace(np.nan, "SKIP")
    agg_rules = {
                'Hal_Utility': 'first',
                'Carla_Utility': 'first',
                "Num_of_sols": 'first',
                "Num_of_min_non_accept": 'first',
                "Min_non_accept": 'first',
                'Total_time': 'mean',
            }
    df = df[cols].groupby('Config_name', sort=False).agg(agg_rules)
    df = df[['Hal_Utility', 'Carla_Utility', 'Num_of_min_non_accept', 'Num_of_sols', 'Min_non_accept', 'Total_time']]
    df = df.round(3)

    SaveDataFrameToTexTemplate(df, f"{er.texTablesFolder}/UtilitarianResults.tex", f"{er.texOutFolder}/Utility_table.tex", row_template_mode=False)
    
    conf_name = list(UtilityConfigs.keys())[3]
    er.plotParetoGraph(conf_name, 0, "Carla", "Hal", 0, fileName=f"ParetoGraph_{conf_name}.png")
    er.plotNonAcceptGraph(conf_name, 0, 0,fileName=f"NaccGraph_{conf_name}.png")

    er.plotTimeChart(UtilityConfigs, 5)



def LegalExperiment():
    defaultConfig = {"Budget": 3, "Horizon": 6}
    
    configs = GenerateConfigs(LegalConfigs, defaultConfig)
    er = ExperimentRunner("LostInsulin", configs)
    Config_repetitions=1
    Environment_repetitions=1
    er.buildEnvironments(configRepetitions=Config_repetitions)
    er.StartServerAndPost(er.makeMdpFileName(configs[2]["Name"], 0))
    input()
    er.run(configRepetitions=Config_repetitions, envRepetitions=Environment_repetitions)
    er.saveResults()
    agg_rules={"Hal": "first", "Necessity":'first', "Legality":'first', "Num_of_min_non_accept": 'first', "Num_of_sols": 'first',
                "Min_non_accept": 'first', 'Total_time': 'mean'}
    df = er.TexSummary(agg_rules=agg_rules, tex_template='LI_LawResults.tex', tex_output='LI_LawResults.tex')
    print(df.head())


LegalExperiment()
input()