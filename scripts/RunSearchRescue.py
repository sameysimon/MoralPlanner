from scripts.AbstractExperiments import ExperimentRunner
from EnvironmentBuilder.SearchRescue.SearchRescueProblem import SearchRescue
from EnvironmentBuilder.SaveEnvironment import SaveEnvToJSON
from EnvironmentBuilder.MDPFactory import makeMDP
from copy import deepcopy
import time
import json
import numpy as np
import ast

runIteratively = True
RealHorizon = 20

# halTheory={"Name":"Hal", "Type":"Utility", "Rank":0}
# consideration = ["Tag", "Theory_Name"|["Theory_Name", "Theory_Name_2"]]

Rawls =  {"Theories": [["Rawls", "Maximin", 0]], "Considerations": [["red:wellbeing", "Rawls"], ["blue:wellbeing", "Rawls"]]}
Fair = {"Theories": [["Fair", "Fairness", 0]], "Considerations": [["red:wellbeing", "Fair"], ["blue:wellbeing", "Fair"]]}
FindInfo = {"Theories": [["FindInfo", "Utility", 0]], "Considerations": [["FindInfo", "Utility"]]}

con = {
    "Name": "SearchRescue",
    "Theories": [["NeverIgnore", "Absolutism", 0], ["FindInfo", "Utility", 1], ["Red_Utility", "Utility", 2], ["Blue_Utility", "Utility", 2]],
    "Considerations": [["FindInfo", "FindInfo"], ["blue:wellbeing", "Blue_Utility"], ["red:wellbeing", "Red_Utility"], ["NeverIgnore", "NeverIgnore"]],
    "Horizon": 5
}
# Creates folder. Starts server.
er = ExperimentRunner("SearchRescue", [con])
er.StartServer()
time.sleep(3)
SearchRescue.GenerateGraph(7)

Trials = 1
results = []
for trial_idx in range(Trials):
    scr_sequence = []
    scr_tag_sequence = []
    action_sequence = []

    curr_state = 0
    scr_props = None
    for i in range(0, RealHorizon - 1):
        mdp = makeMDP("SearchRescue", Theories=con["Theories"], Considerations=con["Considerations"], Horizon=4, initialProps=scr_props)
        mdp.makeAllStatesExplicit()
        mdp_file = f"{er.mdpFolder}/trial{trial_idx}_t{str(i).rjust(2, '0')}.json"
        SaveEnvToJSON(mdp, mdp_file, "SearchRescue")

        fo = f"{er.rawOutFolder}/trial{trial_idx}_t{str(i).rjust(2, '0')}.json"
        dat = er.PostMDPToServer(fileName=mdp_file, fileOut=fo)
        with open(fo) as f:
            d = json.load(f)
            pi_idx = d["Solutions_Order"][0]
            action = d["Solutions"][pi_idx]["Action_Map"]["0"]
            action_sequence.append(action)
            scr_probs = list(map(lambda o : o[0], d["State_transitions"][curr_state][action]))
            rand_scr = np.random.choice(a=range(len(d["State_transitions"][curr_state][action])), p=scr_probs)
            rand_scr = d["State_transitions"][curr_state][action][rand_scr]
            er.CacheSuccessorsOnServer(scrs=[rand_scr], actions=[action])
            scr_sequence.append(rand_scr)
            scr_props = d["State_tags"][rand_scr[1]]
            scr_props = ast.literal_eval(scr_props)
            scr_props["time"] = 0
            scr_tag_sequence.append(scr_props)

    dat = er.GetCachedSuccessorsFromServer()
    results.append(dat.content)

for i in results:
    print(i)

print("\n\n\nFinal Sequence:")
total_prob = 1
prev_state = 0
for i in range(len(scr_sequence)):
    print(f"t={i}. STATE {prev_state} --> {action_sequence[i]} --> {scr_sequence[i][1]} with pr {scr_sequence[i][0]}")
    if i > 0:
        prev_dict = scr_tag_sequence[i-1]
        curr_dict = scr_tag_sequence[i]
        delta = {k: v for k, v in curr_dict.items() if k not in prev_dict or prev_dict[k] != v}
        print(f"   Delta Props: {delta}")

    prev_state = scr_sequence[i][1]
    
    
