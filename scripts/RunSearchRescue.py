from scripts.AbstractExperiments import ExperimentRunner, GenerateConfigs
from EnvironmentBuilder.SearchRescue.SearchRescueProblem import SearchRescue
import EnvironmentBuilder.SearchRescue.Drawing as SearchRescueDraw

from EnvironmentBuilder.SaveEnvironment import SaveEnvToJSON
from EnvironmentBuilder.MDPFactory import makeMDP
from copy import deepcopy
import time
import json
import numpy as np
import ast

RunWithLookahead = True
RealHorizon = 5
Lookahead = 3

# halTheory={"Name":"Hal", "Type":"Utility", "Rank":0}
# consideration = ["Tag", "Theory_Name"|["Theory_Name", "Theory_Name_2"]]

Rawls =  {"Theories": [["Rawls", "Maximin", 0]], "Considerations": [["red:wellbeing", "Rawls"], ["blue:wellbeing", "Rawls"]]}
Fair = {"Theories": [["Fair", "Fairness", 0]], "Considerations": [["red:wellbeing", "Fair"], ["blue:wellbeing", "Fair"]]}
FindInfo = {"Theories": [["FindInfo", "Utility", 0]], "Considerations": [["FindInfo", "Utility"]]}

con = {"Search_Rescue": {
        "Theories": [["NeverIgnore", "Absolutism", 0], ["FindInfo", "Utility", 1], ["Red_Utility", "Utility", 2], ["Blue_Utility", "Utility", 2], ["Prefer_Action", "Utility", 3]],
        "Considerations": [["FindInfo", "FindInfo"], ["blue:wellbeing", "Blue_Utility"], ["red:wellbeing", "Red_Utility"], ["NeverIgnore", "NeverIgnore"], ["Prefer_Action", "Prefer_Action"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}
con2 = {"Search_Rescue": {
        "Theories": [["Positive_Red", "Utility", 0], ["Positive_Blue", "Utility", 0], ["Negative_Red", "Utility", 1], ["Negative_Blue", "Utility", 1], ["Prefer_Action", "Utility", 3]],
        "Considerations": [["blue:pos_wellbeing", "Positive_Blue"], ["red:pos_wellbeing", "Positive_Red"],   ["blue:neg_wellbeing", "Negative_Blue"], ["red:neg_wellbeing", "Negative_Red"], ["Prefer_Action", "Prefer_Action"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}
con3 = {"Search_Rescue": {
        "Theories": [["Positive", "Utility", 0], ["Negative", "Utility", 1], ["Prefer_Action", "Utility", 3]],
        "Considerations": [["pos_wellbeing", "Positive"], ["neg_wellbeing", "Negative"], ["Prefer_Action", "Prefer_Action"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}

# Creates folder. Starts server.


if (not RunWithLookahead):
    configs = GenerateConfigs(con, {})
    er = ExperimentRunner("SearchRescue", configs)
    SearchRescue.BuildMyGraph()
    er.buildEnvironments()
    er.StartServerAndPost(er.makeMdpFileName(configs[0]["Name"], 0))
    input()
    exit()


er = ExperimentRunner("SearchRescue", [con])
er.StartServer()
time.sleep(1)
SearchRescue.GenerateGraph(4,edges=8, base_nodes=1)
#SearchRescue.BuildMyGraph()
SearchRescueDraw.DrawGraph(adj_edge=SearchRescue.AdjEdge, 
                           community=SearchRescue.Community, 
                           node_status=SearchRescue.initialProps["tile_state"],
                           current_node=SearchRescue.initialProps["curr_tile"])


Trials = 1
results = []
for trial_idx in range(Trials):
    scr_sequence = []
    scr_tag_sequence = []
    action_sequence = []
    moral_pols = []

    curr_state = 0
    scr_props = None
    for i in range(0, RealHorizon):
        mdp = makeMDP("SearchRescue", Theories=con["Search_Rescue"]["Theories"], Considerations=con["Search_Rescue"]["Considerations"], Horizon=con["Search_Rescue"]["Horizon"], initialProps=scr_props)
        mdp.makeAllStatesExplicit()
        mdp_file = f"{er.mdpFolder}/trial{trial_idx}_t{str(i).rjust(2, '0')}.json"
        SaveEnvToJSON(mdp, mdp_file, "SearchRescue")

        fo = f"{er.rawOutFolder}/trial{trial_idx}_t{str(i).rjust(2, '0')}.json"
        dat = er.PostMDPToServer(fileName=mdp_file, fileOut=fo, from_data_folder=False)
        with open(fo) as f:
            d = json.load(f)
            moral_pols.append(d['Num_Min_Non_Acceptability'])
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
            if (i==0):
                scr_tag_sequence.append(ast.literal_eval(d["State_tags"][0]))
            scr_tag_sequence.append(scr_props)

    dat = er.GetCachedSuccessorsFromServer()
    dat = dat.json()
    cumulative_worth = dat["Cumulative_Worth"]
    transitions_worth = dat["Total_History"]

    print(f"****Trial {trial_idx}****")

    print("\n\n\nFinal Sequence:")
    total_prob = 1
    prev_state = 0
    for i in range(0, len(scr_sequence)):
        print(f"at t={i}. STATE {prev_state} --> {action_sequence[i]} --> {scr_sequence[i][1]} with pr {scr_sequence[i][0]}")
        print(f"    (of {moral_pols[i]} moral policies)")
        print(f"    transition worth {transitions_worth[i]}")
        print(f"    cumulative worth {cumulative_worth[i]}")
        if i < len(scr_sequence) - 1:
            prev_dict = scr_tag_sequence[i]
            curr_dict = scr_tag_sequence[i + 1]
            delta = {k: v for k, v in curr_dict.items() if k not in prev_dict or prev_dict[k] != v}
            print(f"   Delta Props: {delta}")

        prev_state = scr_sequence[i][1]

    node_hist = [tag["tile_state"] for tag in scr_tag_sequence]
    ani = SearchRescueDraw.AnimateAgent(adj_edge=SearchRescue.AdjEdge,
                                scr_history=scr_sequence,
                                scr_tag_sequence=scr_tag_sequence,
                                community=SearchRescue.Community, 
                                node_status_history=node_hist,
                                action_history=action_sequence,
                                worth_history=transitions_worth,
                                cumulative_worth=cumulative_worth
                                )
ani.save(f"SearchRescue_Trail{trial_idx}.gif", writer="pillow", fps=0.2)

    
