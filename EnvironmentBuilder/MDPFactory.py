from EnvironmentBuilder.TutorProblem.TeacherProblem import TeacherProblem
from EnvironmentBuilder.GridWorld.GridWorldProblem import GridWorldProblem
from EnvironmentBuilder.LostInsulin.LostInsulinProblem import LostInsulin
from EnvironmentBuilder.Elder.ElderProblem import Elder
from EnvironmentBuilder.SaveEnvironment import SaveEnvToJSON
from EnvironmentBuilder.Abstract.AbstractMDP import AbstractMDP

from collections import defaultdict
import argparse
import os


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

domains = {
    "WindyDrone": GridWorldProblem,
    "Tutor": TeacherProblem,
    "LostInsulin": LostInsulin,
    "Abstract": AbstractMDP,
    "Elder": Elder
}

def makeMDP(domain, theories, **kwargs):
    if (not domain in domains.keys()):
        raise Exception(f"Sim: No MMMDP domain '{domain}'.") 
    return domains[domain](theoryClasses=theories, **kwargs)

def buildEnvToFile(domain:str, theories:list, fileOut:str, **kwargs):
    theories = changeTagFormat(theories)
    mdp = makeMDP(domain, theories, **kwargs)
    mdp.makeAllStatesExplicit()
    SaveEnvToJSON(mdp, fileOut)
    print(f"Written MDP to {fileOut} with domain {domain} and theories {theories}.")



def main():
    parser = argparse.ArgumentParser(description="Generate MDP file based on input parameters.")
    parser.add_argument("domain",
                        type=str,
                        required=True,
                        help=f"Generate a domain from list {str(list(domains.keys()))}")

    # Add arguments
    parser.add_argument(
        "-bf", "--branchFactor",
        type=int,
        default=2,
        help="Only for abstract domain. Default=2"
    )
    parser.add_argument(
        "-a", "--actionFactor",
        type=int,
        default=2,
        help="Only for abstract domain. Default=2"
    )
    parser.add_argument(
        "-hz", "--horizon",
        type=int,
        default=4,
        help="Only for abstract domain. Default=2"
    )
    parser.add_argument(
        "-b", "--budget",
        type=int,
        default=4,
        help="Only for SMMDPs. Default=4",
    )
    parser.add_argument(
        "-g,", "--goals",
        type=float,
        default=0,
        help="Only for abstract domain. Probability a state with depth greater than horizon/2 is a goal. If not specified, there will be no goals. Value should be between 0 and 1. Default=0"
    )
    parser.add_argument(
        "-t", "--theories",
        nargs='+',
        required=True,
        help="List of theories with integers (e.g., '0 utility 1 law')",
    )
    parser.add_argument(
        "-s", "--seed",
        type=float,
        default=123,
        help="Seed for random generation",
    )
    parser.add_argument(
        "-o", "--out",
        type=str,
        default=os.getcwd() + "/Data/RandomMDPs/Untitled.json",
        help="Path to the output file (e.g., 'my/file/path/output.json')",
    )

    # Parse arguments
    args = 0
    try:
        args = parser.parse_args()
    except argparse.ArgumentError as e:
        print("Argument Error:")
        print(e)

    # Validate arguments
    if args.branchFactor <= 0:
        parser.error("--branchFactor must be greater than 0.")

    if not (args.goals >= 0 and args.goals <= 1):
        parser.error("--goals must be a probability between 0 and 1.")

    if args.actionFactor <= 0:
        parser.error("--actionFactor must be greater than 0.")

    if args.horizon <= 0:
        parser.error("--horizon must be greater than 1.")

    theoryTags = changeTagFormat(args.theories)
    buildEnvToFile(domain=argparse.domain, theories=theoryTags, fileOut=args.out, kwargs={"horizon":args.horizon, "seed":args.seed, "goals":args.goals, "budget":args.budget, "actionFactor": args.actionFactor, "branchFactor": args.branchFactor} )

if __name__ == "__main__":
    main()
