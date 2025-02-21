from EnvironmentBuilder.TutorProblem.TeacherProblem import TeacherProblem
from EnvironmentBuilder.GridWorld.GridWorldProblem import GridWorldProblem
from EnvironmentBuilder.LostInsulin.LostInsulinProblem import LostInsulin
from EnvironmentBuilder.SaveEnvironment import SaveEnvToJSON
from EnvironmentBuilder.Abstract.AbstractMDP import AbstractMDP

from collections import defaultdict
import argparse
import random
import numpy as np
import datetime
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



def generateRandom(hor, bf, af, tags, b, g, seed, fileName):
    # Generate random environment.
    
    mdp = AbstractMDP(horizon=hor, branchFactor=bf, actionFactor=af, theoryClasses=tags, budget=b, goalOdds=g)
    
    return mdp

def makeMDP(domain, theories, **kwargs):
    # build the mdp and expand all states
    
    if (domain=='WindyDrone'):
        return GridWorldProblem(theoryClasses=theories, **kwargs)
    if (domain=='Teacher'):
        return TeacherProblem(theoryClasses=theories, **kwargs)
    if (domain=='LostInsulin'):
        return LostInsulin(theoryClasses=theories, **kwargs)
    if (domain=='random'):
        return AbstractMDP(theoryClasses=theories, **kwargs)
    print("No domain " + domain)
    return None

def buildEnvToFile(domain:str, theories:list, fileOut:str, **kwargs):
    theories = changeTagFormat(theories)
    mdp = makeMDP(domain, theories, **kwargs)
    mdp.makeAllStatesExplicit()
    SaveEnvToJSON(mdp, fileOut)
    print("Written MDP to {fileOut} with domain {domain} and theories {theories}.")



def main():
    # Create the parser
    parser = argparse.ArgumentParser(description="Generate MDPs based on input parameters.")

    # Add arguments
    parser.add_argument(
        "-bf", "--branchFactor",
        type=int,
        required=True,
        help="Branch factor (must be an integer greater than 0)",
    )
    parser.add_argument(
        "-a", "--actionFactor",
        type=int,
        required=True,
        help="Action factor (must be an integer greater than 0)",
    )
    parser.add_argument(
        "-hz", "--horizon",
        type=int,
        required=True,
        help="Horizon (must be an integer greater than 1)",
    )
    parser.add_argument(
        "-b", "--budget",
        type=int,
        default=0,
        help="Budget (must be an integer greater than 0). Optional.",
    )
    parser.add_argument(
        "-g,", "--goals",
        type=float,
        default=0,
        help="Probability a state with depth greater than horizon/2 is a goal. If not specified, there will be no goals. Value should be between 0 and 1.",
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
    generateRandom(args.horizon, args.branchFactor, args.actionFactor, theoryTags, args.budget, args.goals, args.out)

if __name__ == "__main__":
    main()
