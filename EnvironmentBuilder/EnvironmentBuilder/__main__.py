from EnvironmentBuilder.TutorProblem.TeacherProblem import TeacherProblem
from EnvironmentBuilder.GridWorld.GridWorldProblem import GridWorldProblem
from EnvironmentBuilder.LostInsulin.LostInsulinProblem import LostInsulin
import EnvironmentBuilder.CustomJSON as myJSON
import argparse
import os
import ast

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

os.system('color')


parser = argparse.ArgumentParser()

args = parser.add_argument("--policy", type=str, default='', help='Load a policy to output to file and console.')

args = parser.add_argument("--domain", type=str, default='WindyDrone', help='Select the environment to use')

args = parser.add_argument("--out", type=str, default=os.path.dirname(os.path.abspath(__file__)) + '/outputs/out.json', help='Output `.json` file. Defaults to outputs/out.json')

args = parser.add_argument("--theoryTags", type=str, default='[[]]', help='The theory configuration.')

args = parser.parse_args()

theoryTags = [['avoid_playgrounds', "avoid_checkpoints"]]#ast.literal_eval(args.theoryTags)
mdp=0
# build the mdp and expand all states
if (args.domain=='WindyDrone'):
    mdp = GridWorldProblem(theoryClasses=theoryTags)
elif (args.domain=='Teacher'):
    mdp = TeacherProblem(theoryClasses=theoryTags)
elif (True or args.domain=='LostInsulin'):
    mdp = LostInsulin(theoryClasses=theoryTags)
    
mdp = LostInsulin(theoryClasses=[['LifeAndDeath']])
mdp.makeAllStatesExplicit()

actionList = [a for a in mdp.actions]

if (args.policy!=''):
    with open(args.policy, 'r') as file:  # 'output.json' is the name of your file
        json_policy = myJSON.json.load(file)['policy']
        pi = ""
        for i in json_policy:
            sIdx, time, action = i[0], i[1], i[2]
            pi += str(mdp.states[sIdx]) + "@" + str(time) + " --> " + str(actionList[action]) + "\n"
            pi += "   " + str(mdp.states[sIdx].props) + "\n"
        


# The JSON output structure
output = {'total_states': len(mdp.states), 'actions': actionList}


state_transitions = []
output['goals']=[]

for s_idx in range(len(mdp.states)):
    s = mdp.states[s_idx]
    if (mdp.isGoal(s)):
        output['goals'].append(s_idx)
    actionTransitionsMap = {}
    for a in mdp.getActions(s):
        transitionList = []
        for scr in mdp.getActionSuccessors(s, a):
            transition = [scr.probability, mdp.CostTheory.judge(scr), scr.targetState.id]
            for theory in mdp.Theories:
                transition.append(theory.judge(scr))
            transitionList.append(transition)
        actionTransitionsMap[a] = transitionList
    state_transitions.append(actionTransitionsMap)
# I know, I know...

output['state_transitions'] = state_transitions

theories = []
for theory in mdp.Theories:
    currTheory = {}
    currTheory['Name'] = theory.tag
    currTheory['Type'] = theory.type
    currTheory['Heuristic'] = []
    for s in mdp.states:
        currTheory['Heuristic'].append(theory.StateHeuristic(s))
    currTheory['Default'] = theory.default
    currTheory['Rank'] = theory.rank
    theories.append(currTheory)
output['theories']=theories


directory = os.path.dirname(args.out)
t = os.path.dirname(os.path.abspath(__file__))


json_string = myJSON.custom_json_format(output)
with open(args.out, 'w') as file:  # 'output.json' is the name of your file
    file.write(json_string)
    #json.dump(output, file, indent=2, cls=cJE)

def printHeuristics(mdp, state):
    s=""
    for t in mdp.Theories:
        s += "tag={}, Heuristic={} :: ".format(t.tag, t.StateHeuristic(state))
    return s

def printProps(props, propNames=[], showOnlyChanges=False, oldProps={}):
    if (len(propNames)==0):
        propNames = list(props.keys())
    if (not any(pn in props.keys() for pn in propNames)):
        return props
    out="{"
    for idx in range(len(propNames)-1):
        if (showOnlyChanges and props[propNames[idx]]==oldProps[propNames[idx]]):
            continue
        out += str(propNames[idx]) + ": " + str(props[propNames[idx]]) + ", "
    out += str(propNames[-1]) + ": " + str(props[propNames[-1]]) + "}"
    return out

def getStateByProps():
    print("Sample props below: ")
    sample = mdp.states[0].props
    print(printProps(sample))
    props = {}
    while True:
        key = str(input("Select a property to specify. Q to quit."))
        if (key=="Q"):
            print("Aborting select by props.")
            return ""
        if (key in sample.keys()):
            val_ = str(input("Specify a value"))
            val = ast.literal_eval(val_)
            print('interpreted {} as a {}'.format(val, type(val)))
            props[key] = val
        else:
            print("property not found.")
        for s in mdp.states:
            if (s.props[key]==props[key]):
                return str(s.id)

        print("No such state found.")

chosenProps = []
choice = 0
onlyShowChanges=True
print("There are {} states, {} actions and {} theories.".format(len(mdp.states), len(output['actions']), len(mdp.Theories)))
while (choice!="Q"):
    choice = str(input("Inspect state with ID? Optionally, specify props to display. To remove selected props, just enter clear. Toggle show only changed props with 'delta'. Use command props to select state by props (Q to exit). ")).split(" ")
    if (len(choice)==0):
        continue
    nextState = choice.pop(0)
    if (nextState=="Q" or nextState=="q"):
        break
    if nextState=="props":
        nextState = getStateByProps()
        if nextState=="":
            continue
    
    if "delta" in choice:
        onlyShowChanges = not onlyShowChanges
        
    if "clear" in choice:
        chosenProps = []
    elif len(choice)!=0:
        chosenProps=choice

    # print state details
    if not nextState.isnumeric():
        print("state id {} not numeric.".format(nextState))
        continue
    nextState = int(nextState)
    if (not (0 <= nextState and nextState < len(mdp.states))):
        print("state id {} not in range 0 to {}".format(nextState, len(mdp.states)))
        continue

    state = mdp.states[nextState]
    print(f"**** State ID {bcolors.OKBLUE}{state.id}{bcolors.ENDC} ****")
    print("** Theory Heuristics")
    print(printHeuristics(mdp,state))
    print("** Properties:")
    print(printProps(state.props, chosenProps, False))
    print("** ACTIONS:")
    for a in mdp.getActions(state):
        print(f" Action {bcolors.OKBLUE}{a}{bcolors.ENDC}:")
        for successor in mdp.getActionSuccessors(state, a, True):
            print("     - chance={}; cost={}; To state={}{}{} with {}".format(round(successor.probability,4), mdp.CostTheory.judge(successor), bcolors.OKBLUE, successor.targetState.id, bcolors.ENDC, printProps(successor.targetState.props, chosenProps, onlyShowChanges, state.props)))
            for t in mdp.Theories:
                print("         Theory with tag {} values at {}".format(t.tag, t.judge(successor)))
        print("     DONE ACTION {}".format(a))
    print("DONE ACTIONS FOR {}".format(state.id))
    print("\n\n")


