import json
from EnvironmentBuilder.BaseMDP import MDP

class PolicyProcessor():
    def __init__(self, fileName:str):
        with open(fileName, 'r') as file:
            self.data = json.load(file)
            print("{} opened successfully.".format(fileName))

    def getTheories(self):
        return self.data["Theories"]

    def printPolicy(self, actionList:list, mdp):
        pi = ""
        for sol in self.data["solutions"]:
            for key, value in sol['Action_Map'].items():
                time, sIdx = int(key.split(',')[0]), int(key.split(',')[1])
                action = int(value)
                pi += str(mdp.states[sIdx].props) + "@" + str(time) + " --> " + str(actionList[action]) + "\n"
                pi += "   " + str(mdp.states[sIdx].props) + "\n"
            pi+="\n\n\n\n"
        return pi
    
    def printPolicySummary(self, count):
        s = "Non-accept: {}; ".format(self.data["solutions"][count])
        for key, value in self.data["solutions"][count]["Expectation"]:
            s += "{}: {}; ".format(key, value)
        s.removesuffix("; ")
        return s

    def getPolicyActions(self,policyID, state, actionList):
        s = ""
        for key, value in self.data["solutions"][policyID]["Action_Map"].items():
            if (f",{state}" in key):
                s+= f"{key} -> {value} ({actionList[int(value)]})"
        return s

    def policyCount(self):
        return len(self.data["solutions"])

    