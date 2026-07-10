from scripts.AbstractExperiments import ExperimentRunner, GenerateConfigs
from EnvironmentBuilder.SearchRescue.Rescue import Rescue
import EnvironmentBuilder.SearchRescue.Drawing as SearchRescueDraw
from scripts.TexTables import SaveDataFrameToTexTemplate
from copy import deepcopy
import pandas as pd
import numpy as np
import json
import matplotlib.pyplot as plt

plt.rcParams.update({
    "font.family": "Charter",
    "font.size": 16,
    "axes.titlesize": 16,
    "axes.labelsize": 16,
    "xtick.labelsize": 14,
    "ytick.labelsize": 14,
    "legend.fontsize": 14,
})
def latex_config_name(name):
    return "$" + name.replace("^0", "^{0}") + "$"




def GenerateTeamConfigs(teams_:list, lookahead_:int):
    configs = {}
    # Additive
    con = {"Theories": [["Add_Util", "Utility", 0]], "Considerations": [], "Horizon":lookahead_}
    for t in teams_:
        con["Considerations"].append([f"{t}:wellbeing", "Add_Util"])
    configs[f"{len(teams_)}_add"] = con

    # Balance
    con = {"Theories": [], "Considerations": [], "Horizon":lookahead_}
    for t in teams_:
        con["Theories"].append([f"{t}", "Utility", 0])
        con["Considerations"].append([f"{t}:wellbeing", f"{t}"])
    configs[f"{len(teams_)}_bal"] = con

    # Fairness
    con = {"Theories": [["Fair", "Fairness", 0]], "Considerations": [], "Horizon":lookahead_}
    for t in teams_:
        con["Considerations"].append([f"{t}:wellbeing", ["Fair"]])
    configs[f"{len(teams_)}_fair"] = con

    # Fairness + Others
    con = {"Theories": [["Fair", "Fairness", 0]], "Considerations": [], "Horizon":lookahead_}
    for t in teams_:
        con["Theories"].append([f"{t}", "Utility", 0])
        con["Considerations"].append([f"{t}:wellbeing", [f"{t}", "Fair"]])
    configs[f"{len(teams_)}_fair_bal"] = con

    # Fairness + additive
    con = {"Theories": [["Fair", "Fairness", 0]], "Considerations": [], "Horizon":lookahead_}
    for t in teams_:
        con["Theories"].append([f"{t}", "Utility", 0])
        con["Considerations"].append([f"{t}:wellbeing", [f"{t}", "Fair"]])
    configs[f"{len(teams_)}_fair_bal"] = con

    # Rawls
    con = {"Theories":[["Rawls", "Maximin", 0]], "Considerations": [], "Horizon":lookahead_}
    for t in teams_:
        con["Considerations"].append([f"{t}:wellbeing", ["Rawls"]])
    configs[f"{len(teams_)}_rawl"] = con

    # Rawls + Others
    con = {"Theories": [["Rawls", "Maximin", 0]], "Considerations": [], "Horizon":lookahead_}
    for t in teams_:
        con["Theories"].append([f"{t}", "Utility", 0])
        con["Considerations"].append([f"{t}:wellbeing", [f"{t}", "Rawls"]])
    configs[f"{len(teams_)}_rawl_bal"] = con

    return configs

teams = ['red', 'blue']
horizon = 6
configs = GenerateConfigs(GenerateTeamConfigs(teams, horizon), {})
er = ExperimentRunner("Rescue", configs, MoralPlanner_Location="/./")
w = [2**i for i in range(len(teams))]
total = sum(w)
w = [i / total for i in w]
o = {}
for t in teams:
    for i, t in enumerate(teams):
        o[f"{t}_hosp_success"] = w[i]

mdps = er.buildEnvironments()

SearchRescueDraw.DrawGraph(adj_edge=mdps["2_add"][0].AdjEdge, 
                           community=mdps["2_add"][0].Community, 
                           curr_props=mdps["2_add"][0].initialProps)

er.run(envRepetitions=1)


df = pd.DataFrame(er.data)
df = df.drop(labels=['Conf_rep', 'Theories', 'Considerations', 'CQ1_time', 'CQ2_time', 'Out_time', 'Sol_reduce_time'], axis=1)
df = df.replace(["", "N/A", "NA", "nan", "None"], np.nan)

cols = [f"{t}:wellbeing" for t in teams]
df[cols] = df[cols].apply(pd.to_numeric, errors='coerce', axis=1)
df['range'] = df[cols].max(axis=1) - df[cols].min(axis=1)
df['Sum wellbeing'] = df[cols].sum(axis=1)


agg_rules = {}
for c in cols:
    agg_rules[c] = 'first'
agg_rules = agg_rules | {
    "range": 'first',
    "Sum wellbeing": 'first',
    "Num_of_min_non_accept": 'first',
    "Num_of_sols": 'first',
    "Min_non_accept": 'first',
    'Total_time': 'mean',
}

df = df.groupby('Config_name', sort=False).agg(agg_rules)
df = df[cols + ['range', 'Sum wellbeing', 'Num_of_min_non_accept', 'Num_of_sols', 'Min_non_accept', 'Total_time']]

print(df.head(1000))