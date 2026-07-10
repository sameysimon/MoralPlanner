from scripts.AbstractExperiments import ExperimentRunner, GenerateConfigs
from EnvironmentBuilder.SearchRescue.Rescue import Rescue
import EnvironmentBuilder.SearchRescue.Drawing as SearchRescueDraw

from EnvironmentBuilder.SaveEnvironment import SaveEnvToJSON
from EnvironmentBuilder.MDPFactory import makeMDP
from copy import deepcopy
import time
import json
import numpy as np

RunWithLookahead = False
RealHorizon = 20
Lookahead = 4

Rawls =  {"Theories": [["Rawls", "Maximin", 0]], "Considerations": [["red:wellbeing", "Rawls"], ["blue:wellbeing", "Rawls"]]}
Fair = {"Theories": [["Fair", "Fairness", 0]], "Considerations": [["red:wellbeing", "Fair"], ["blue:wellbeing", "Fair"]]}
FindInfo = {"Theories": [["FindInfo", "Utility", 0]], "Considerations": [["FindInfo", "Utility"]]}


additive = {"Additive": {
        "Theories": [["Add_Util", "Utility", 0]],
        "Considerations": [["red:wellbeing", "Add_Util"], ["blue:wellbeing", "Add_Util"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}

balance = {"Equal": {
        "Theories": [["Red", "Utility", 0], ["Blue", "Utility", 0]],
        "Considerations": [["red:wellbeing", "Red"], ["blue:wellbeing", "Blue"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}

fair = {"Fairness": {
        "Theories": [["Fair", "Fairness", 0]],
        "Considerations": [["red:wellbeing", "Fair"], ["blue:wellbeing", "Fair"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}

rawls = {"Rawls": {
        "Theories": [["Rawls", "Maximin", 0]],
        "Considerations": [["red:wellbeing", "Rawls"], ["blue:wellbeing", "Rawls"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}

all = {"All": {
        "Theories": [["Red", "Utility", 0], ["Blue", "Utility", 0], ["Rawls", "Maximin", 0], ["Fair", "Fairness", 0]],
        "Considerations": [["red:wellbeing", ["Red", "Rawls", "Fair"]], ["blue:wellbeing", ["Blue", "Rawls", "Fair"]]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}


s_balance = {"Search": {
        "Theories": [["Red", "Utility", 0], ["Blue", "Utility", 0]],
        "Considerations": [["red:search", "Red"], ["blue:search", "Blue"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}
sr_balance = {"Search": {
        "Theories": [["Red", "Utility", 0], ["Blue", "Utility", 0]],
        "Considerations": [["red:search", "Red"], ["blue:search", "Blue"], ["red:wellbeing", "Red"], ["blue:wellbeing", "Blue"]],
        "Horizon": Lookahead if RunWithLookahead else RealHorizon
    }
}


# Creates folder. Starts server.

if (not RunWithLookahead):
    configs = GenerateConfigs(s_balance, {})
    er = ExperimentRunner("Rescue", configs)

    Rescue.BuildMyGraph(['red', 'blue', 'green'], 3)
    SearchRescueDraw.DrawGraph(adj_edge=Rescue.AdjEdge, community=Rescue.Community, curr_props=Rescue.initialProps)
    er.buildEnvironments()
    er.StartServerAndPost(er.makeMdpFileName(configs[0]["Name"], 0))
    #er.PostMDPToServer(er.makeMdpFileName(configs[0]["Name"], 0))
    input()
    exit()



configs = GenerateConfigs(s_balance, {})
er = ExperimentRunner("Rescue", configs)
er.StartServer()
time.sleep(1)
Rescue.BuildMyGraph(10)

#Rescue.BuildMyGraph()



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
        mdp = makeMDP("Rescue",
                      Theories=configs[0]["Theories"],
                      Considerations=configs[0]["Considerations"],
                      Horizon=configs[0]["Horizon"], 
                      initialProps=scr_props)
        mdp.makeAllStatesExplicit()
        mdp_file = f"{er.mdpFolder}/trial{trial_idx}_t{str(i).rjust(2, '0')}.json"
        SaveEnvToJSON(mdp, mdp_file, "Rescue")

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

    ani = SearchRescueDraw.AnimateAgent(adj_edge=Rescue.AdjEdge,
                                scr_history=scr_sequence,
                                scr_tag_sequence=scr_tag_sequence,
                                community=Rescue.Community, 
                                action_history=action_sequence,
                                worth_history=transitions_worth,
                                cumulative_worth=cumulative_worth
                                )
ani.save(f"Rescue_Trail{trial_idx}.gif", writer="pillow", fps=0.2)

    
