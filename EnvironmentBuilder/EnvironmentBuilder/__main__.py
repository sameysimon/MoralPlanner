from EnvironmentBuilder.TutorProblem.TeacherProblem import TeacherProblem
from EnvironmentBuilder.GridWorld.GridWorldProblem import GridWorldProblem
from EnvironmentBuilder.LostInsulin.LostInsulinProblem import LostInsulin
from EnvironmentBuilder.OpenPolicy import PolicyProcessor
from EnvironmentBuilder.SaveEnvironment import SaveEnvToJSON

from collections import defaultdict
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


def makeMDP(theoryTags):
    # build the mdp and expand all states
    if (args.domain=='WindyDrone'):
        return GridWorldProblem(theoryClasses=theoryTags)
    if (args.domain=='Teacher'):
        return TeacherProblem(theoryClasses=theoryTags)
    if (args.domain=='LostInsulin'):
        return LostInsulin(theoryClasses=theoryTags)
    print("No domain " + args.domain)
    return None


#
# Process Arguments
#

parser = argparse.ArgumentParser()

parser.add_argument('--policy', type=str, help='File location for the policy')

args = parser.add_argument("--domain", type=str, default='WindyDrone', help='Select the environment to use')

args = parser.add_argument("--out", type=str, default=os.path.dirname(os.path.abspath(__file__)) + '/outputs/out.json', help='Output `.json` file. Defaults to outputs/out.json')

args = parser.add_argument("--explore", action='store_true', help='Explore the environment')

parser.add_argument('--theoryTags', metavar='PAIR', type=str, nargs='+',
                        default=None, help="Any number of integer-string pairs")

args = parser.parse_args()

#
# Check if processing a policy.
#
p = 0
if args.policy:
    p = PolicyProcessor(args.policy)


# 
# Derive theory tags from commands, defaults or policy/
#
theoryTags = args.theoryTags

if p!=0:
    theoryTags = p.getTheories()

if theoryTags is None:
    print("Using default theory arguments.")
    if (args.domain=='WindyDrone'):
        theoryTags = ["0", "Cost", "0", "avoid_playgrounds"]
    elif (args.domain=='LostInsulin'):
        theoryTags =  ["0", 'CarlaLife', "0", 'HalLife']#["0", "LifeAndDeath", "0", 'ToSteal']
    else:
        print("No such moral theory.")

# Make theories into desired format for MDP
pairs = []
for i in range(0, len(theoryTags), 2):
        try:
            integer = int(theoryTags[i])
            string = theoryTags[i+1]
            pairs.append((integer, string))
        except ValueError:
            raise ValueError(f"Expected an integer at position {i}, but got {args.pairs[i]}")

sorted(pairs, key=lambda x: x[0])

grouped_strings = defaultdict(list)
for integer, string in pairs:
    grouped_strings[integer].append(string)

theoryTags = [group for group in grouped_strings.values()]


#
# Make the MDP
#
mdp=makeMDP(theoryTags)
if (mdp==None):
    exit(1)


#
# Make the MDP Explicit. (Build all states+transitions of the MDP)
#
mdp.makeAllStatesExplicit()

actionList = SaveEnvToJSON(mdp, args.out)
if p!=0:
    print("POLICY LIST")
    p.printPolicy(actionList)

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
print("There are {} states, {} actions and {} theories.".format(len(mdp.states), len(actionList), len(mdp.Theories)))
if (not args.explore):
    exit(0)
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
        costs, theoryH = [], {}
        probs = []
        print(f" Action {bcolors.OKBLUE}{a}{bcolors.ENDC}:")
        for successor in mdp.getActionSuccessors(state, a, True):
            print("     - chance={}; cost={}; To state={}{}{} with {}".format(round(successor.probability,4), mdp.CostTheory.judge(successor), bcolors.OKBLUE, successor.targetState.id, bcolors.ENDC, printProps(successor.targetState.props, chosenProps, onlyShowChanges, state.props)))
            costs.append(mdp.CostTheory.judge(successor))
            probs.append(successor.probability)
            for t in mdp.Theories:
                print("         Theory with tag {} values at {}, H={}".format(t.tag, t.judge(successor), t.StateHeuristic(successor.targetState)))
                theoryH.setdefault(t.tag, []).append(t.StateHeuristic(successor.targetState))

        print("     DONE ACTION {}".format(a))
        theoryStr = ""
        for tag, l in theoryH.items():
            theoryStr += tag + "->" + str(round(sum([theoryH[tag][i] * probs[i] for i in range(len(probs))]), 4)) + ";  "
        print("         Action Avg Cost={}, Theory Heuristics={}".format(round(sum([costs[i] * probs[i] for i in range(len(probs))]), 4), theoryStr))
    print("DONE ACTIONS FOR {}".format(state.id))
    print("\n\n")


