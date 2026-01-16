from EnvironmentBuilder.BaseMDP import MDP
import EnvironmentBuilder.CustomJSON as myJSON
import os

def SaveEnvToJSON(mdp: MDP, fileName:str, domainName:str):
    actionList = [a for a in mdp.actions]

    # The JSON output structure
    output = {'Domain': domainName, 'Total_states': len(mdp.states), 'Actions': actionList, 'Horizon': mdp.horizon+1, 'Goals': [], 'State_tags': [], 'State_time': []}

    # State transitions
    state_transitions = []
    for s_idx in range(len(mdp.states)):
        s = mdp.states[s_idx]

        # Add time stamp
        output['State_time'].append(s.props['time'])
        # Add if a goal
        if (mdp.isGoal(s)):
            output['Goals'].append(s_idx)
        # Add state Tag
        output['State_tags'].append(mdp.stateString(s))
        # Add transitions for each goal
        actionTransitionsMap = {}
        for a in mdp.getActions(s):
            transitionList = []
            for scr in mdp.getActionSuccessors(s, a):
                transition = [scr.probability, scr.targetState.id]
                for con in mdp.Considerations:
                    transition.append(con.judge(scr))
                transitionList.append(transition)
            actionTransitionsMap[a] = transitionList
        state_transitions.append(actionTransitionsMap)
    output['State_transitions'] = state_transitions

    

    # Add Theories
    theories = []
    for t in mdp.Theories:
        currThy = {"Name":t.name, "Type":t.type, "Rank":t.rank}
        theories.append(currThy)
    output['Theories']=theories

    considers = []
    for c in mdp.Considerations:
        currCon = {}
        currCon['Name'] = c.tag
        currCon['Type'] = c.type
        if (c.type=="Threshold"):
            currCon['Threshold'] = c.threshold
        if (c.type=="Cost"):
            currCon['Budget'] = mdp.budget
        if (c.type=="Ordinal"):
            currCon['Optimality_type'] = c.optimalityType
            currCon['Ordinal_labels'] = c.ordinalLabels
        currCon["Component_of"] = c.componentOf
        currCon['Default'] = c.default
            
        currCon['Heuristic'] = []
        for s in mdp.states:
            currCon['Heuristic'].append(c.StateHeuristic(s))
        considers.append(currCon)

    output['Considerations']=considers


    t = os.path.dirname(os.path.abspath(__file__))

    json_string = myJSON.custom_json_format(output)
    with open(fileName, 'w') as file:
        file.write(json_string)
        #json.dump(output, file, indent=2, cls=cJE)
    print('Written to `' + fileName + '`')
    return actionList # return action list because it's handy to have lol