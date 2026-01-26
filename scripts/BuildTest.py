import scipy
import numpy as np
import json
import os
import EnvironmentBuilder.CustomJSON as myJSON

def clamp(num, min_, max_):
    if num<min_:
        return min_
    if num>max_:
        return max_
    return num

def Build(fileName:str, theories=1, numOfActions=100, minBranches=3, maxBranches=10, numOfRanks=1):
    transitions = {}
    solutions = [{"Action_Map":{}, "Acceptability":0,"Expectation":{}} for _ in range(numOfActions)]
    actionAccept = [0] * numOfActions
    assert(numOfRanks <= theories)
    ranks = []
    ranks += list(range(numOfRanks))
    if (numOfRanks > 1):
        ranks += [np.random.randint(1, numOfRanks) for _ in range(theories - numOfRanks)]
    else:
        ranks += [0 for _ in range(theories - numOfRanks)]
    ranks = sorted(ranks)
    assert(len(ranks)==theories)
    totalStates = 2
    for aIdx in range(numOfActions):
        solutions[aIdx]["Action_Map"]["0"] = str(aIdx)
        numOfBranches = np.random.randint(minBranches,maxBranches)
        probs = scipy.stats.dirichlet.rvs([0.7] * numOfBranches)[0]
        transitions[str(aIdx)] = [[probs[i], 1] for i in range(numOfBranches)]
        #totalStates+=numOfBranches
        x = sum(probs)
        assert(0.990 < x and x < 1.001)
    
    expUtils = np.zeros([theories, numOfActions])
    maxUtils = np.zeros([theories, numOfActions])
    threshold = 0
    agileActions = []
    for thIdx in range(theories):
        
        greaterTheories = list(filter(lambda t_ : ranks[t_] < ranks[thIdx], range(theories)))
        shuffledActions = list(range(numOfActions))
        np.random.shuffle(shuffledActions)
        x = list(range(numOfActions, 0, -1)) 
        for a_idx in range(len(shuffledActions)):
            a = shuffledActions[a_idx]
            expUtils[thIdx][a] = x[a_idx]
        for i in range(numOfActions):
            currA = shuffledActions[i]
            if i>0:
                agileActions = list(filter(lambda a_ : all(expUtils[t][a_] >= expUtils[t][currA] and currA!=a_ for t in greaterTheories), shuffledActions[:i]))
                
                if len(agileActions)>0:
                    threshold = max([maxUtils[thIdx][a_] for a_ in agileActions ])


            maxUtils[thIdx][currA] = AssignUtilities(transitions, currA, actionAccept, expUtils[thIdx][currA], trackNonAccept=i>0 and len(agileActions)>0, attackThreshold=threshold)
            solutions[currA]["Expectation"][f"C_{thIdx}"] = expUtils[thIdx][currA]


    for aIdx in range(numOfActions):
        solutions[aIdx]["Acceptability"] = actionAccept[aIdx]
        for thIdx in range(theories):
            solutions[i]["Expectation"][f"M_{thIdx}"] = expUtils[thIdx][i]

    output = {"Horizon": 1, 
              "Total_states": totalStates, 
              "Actions":[str(aIdx) for aIdx in range(numOfActions)],
              "Theories": [{"Name":f"M_{thIdx}", "Type":"Utility", "Rank": ranks[thIdx]} for thIdx in range(theories)],
              "Considerations": [{"Name":f"C_{thIdx}", "Type": "Utility", "Component_of":f"M_{thIdx}"} for thIdx in range(theories)],
              "Min_nacc": min(actionAccept),
              "Min_nacc_policies": list(filter(lambda a_ : actionAccept[a_]==min(actionAccept),range(numOfActions))),
              "State_time": [0,1],
              "State_transitions":[transitions, {}],
              "Solutions":solutions,
              }
    
    fn = os.getcwd() + f"/Data/MDPs/Tests/{fileName}"
    if (not fn.endswith(".json")):
        fn += ".json"
    json_string = myJSON.custom_json_format(output)
    with open(fn, 'w') as file:
        file.write(json_string)
    print('Written to `' + fn + '`')
    return output

    




def AssignUtilities(transitions, action:int, actionAccept:list, ExpUtil:float, trackNonAccept:bool, attackThreshold:float):
    numOfBranches = len(transitions[str(action)])
    utilities = [np.random.uniform(0, 10) for _ in range(numOfBranches)]
    probs = [transitions[str(action)][i][0] for i in range(numOfBranches)]
    currentExpUtil = sum(utilities[i] * probs[i] for i in range(numOfBranches))
    scale_factor = ExpUtil / currentExpUtil if currentExpUtil > 0 else 1
    utilities = [u * scale_factor for u in utilities]
    for i in range(numOfBranches):
        transitions[str(action)][i].append(utilities[i])
    # check exp utility is correct
    actualUtil = sum([transitions[str(action)][i][0] * transitions[str(action)][i][-1] for i in range(numOfBranches)])
    assert(ExpUtil - 0.001 < actualUtil and actualUtil < ExpUtil + 0.001)
    # Sum probabilities whose utility is under threshold
    if trackNonAccept:
        actionAccept[action] += sum([probs[i] for i in 
                                 filter(lambda i : utilities[i] < attackThreshold, 
                                                          range(len(utilities)))])

    return max(utilities)

np.random.seed(seed=4+8+15+16+23+42)

o = Build("test_1Theory",theories=1)
assert(o["Min_nacc"]==0)
print("Built test_1Theory")
o = Build("test_5Theory",theories=5)
print("Built test_5Theory")
o= Build("test_10Theory",theories=10)
print("Built test_10Theory")


o= Build("test_100Theory",theories=100)
print("Built test_100Theory")
o= Build("test_200Theory",theories=200)
print("Built test_200Theory")
o= Build("test_500Theory",theories=500)
print("Built test_500Theory")
o= Build("test_1000Theory",theories=1000)
print("Built test_1000Theory")

o= Build("test_100Theory_25Rank",theories=100, numOfRanks=25)
print("Built test_100Theory_25Rank")
o= Build("test_100Theory_50Rank",theories=100, numOfRanks=50)
print("Built test_100Theory_50Rank")
o= Build("test_100Theory_75Rank",theories=100, numOfRanks=75)
print("Built test_100Theory_75Rank")
o= Build("test_100Theory_100Rank",theories=100,numOfRanks=100)
print("Built test_100Theory_100Rank")

o = Build("test_2Theory_2Rank_3Actions",theories=2, numOfRanks=2, numOfActions=3)
print("Built test_2Theory_2Rank_3Actions")

print("Done!")
