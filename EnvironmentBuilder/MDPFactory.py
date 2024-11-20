from EnvironmentBuilder.TutorProblem.TeacherProblem import TeacherProblem
from EnvironmentBuilder.GridWorld.GridWorldProblem import GridWorldProblem
from EnvironmentBuilder.LostInsulin.LostInsulinProblem import LostInsulin
from EnvironmentBuilder.SaveEnvironment import SaveEnvToJSON
from collections import defaultdict


# Make theories into desired format for MDP
# Change from [0, theoryOne, 1, theoryTwo] --> [[theoryOne], [theoryTwo]]
def changeTagFormat(theoryTags):
    pairs = []
    for i in range(0, len(theoryTags), 2):
            try:
                integer = int(theoryTags[i])
                string = theoryTags[i+1]
                pairs.append((integer, string))
            except ValueError:
                raise ValueError(f"Expected an integer at position {i}, but got {theoryTags[i]}")

    sorted(pairs, key=lambda x: x[0])

    grouped_strings = defaultdict(list)
    for integer, string in pairs:
        grouped_strings[integer].append(string)

    theoryTags = []
    while (len(grouped_strings)>0):
        x = grouped_strings.pop(min(grouped_strings.keys()))
        theoryTags.append(x)
    return theoryTags


def makeMDP(domain, theoryTags):
    # build the mdp and expand all states
    if (domain=='WindyDrone'):
        return GridWorldProblem(theoryClasses=theoryTags)
    if (domain=='Teacher'):
        return TeacherProblem(theoryClasses=theoryTags)
    if (domain=='LostInsulin'):
        return LostInsulin(theoryClasses=theoryTags)
    print("No domain " + domain)
    return None

def buildEnvToFile(domain:str, theories:list, fileOut:str):
    theories = changeTagFormat(theories)
    mdp = makeMDP(domain, theories)
    mdp.makeAllStatesExplicit()
    SaveEnvToJSON(mdp, fileOut)
    print("Written to {fileOut} with domain {domain} and theories {theories}.")