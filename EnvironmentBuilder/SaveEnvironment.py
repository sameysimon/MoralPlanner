from EnvironmentBuilder.BaseMDP import MDP
import EnvironmentBuilder.CustomJSON as myJSON
import os

def SaveEnvToJSON(mdp: MDP, fileName:str):
    actionList = [a for a in mdp.actions]

    # The JSON output structure
    output = {'total_states': len(mdp.states), 'actions': actionList, 'horizon': mdp.horizon+1, 'goals': [], 'state_tags': [], 'state_time': []}

    # State transitions
    state_transitions = []
    for s_idx in range(len(mdp.states)):
        s = mdp.states[s_idx]

        # Add time stamp
        output['state_time'].append(s.props['time'])
        # Add if a goal
        if (mdp.isGoal(s)):
            output['goals'].append(s_idx)
        # Add state Tag
        output['state_tags'].append(mdp.stateString(s))
        # Add transitions for each goal
        actionTransitionsMap = {}
        for a in mdp.getActions(s):
            transitionList = []
            for scr in mdp.getActionSuccessors(s, a):
                transition = [scr.probability, scr.targetState.id]
                for theory in mdp.Theories:
                    transition.append(theory.judge(scr))
                transitionList.append(transition)
            actionTransitionsMap[a] = transitionList
        state_transitions.append(actionTransitionsMap)
    # I know, I know...
    output['state_transitions'] = state_transitions

    

    # Add Theories
    theories = []
    for theory in mdp.Theories:
        currTheory = {}
        currTheory['Name'] = theory.tag
        currTheory['Type'] = theory.type
        if (theory.type=="Threshold"):
            currTheory['Threshold'] = theory.threshold
        if (theory.type=="Cost"):
            currTheory['Budget'] = mdp.budget
        currTheory['Heuristic'] = []
        for s in mdp.states:
            currTheory['Heuristic'].append(theory.StateHeuristic(s))
        currTheory['Default'] = theory.default
        currTheory['Rank'] = theory.rank
        theories.append(currTheory)

    output['theories']=theories


    directory = os.path.dirname(fileName)
    t = os.path.dirname(os.path.abspath(__file__))

    json_string = myJSON.custom_json_format(output)
    with open(fileName, 'w') as file:
        file.write(json_string)
        #json.dump(output, file, indent=2, cls=cJE)
    print('Written to `' + fileName + '`')
    return actionList # return action list because it's handy to have lol