import random
import math
import CustomJSON as myJSON
from scipy.optimize import linprog
import os

class Builder:
    def __init__(self):
        self.output = {}
        self.Policies = []
    
    def addTheory(self, name="Utilitarian", theoryType="Utility", rank=0):
        self.output.setdefault("theories", [])
        x = len(self.output["theories"])
        t = {
            "Name": name,
            "Type": theoryType,
            "Rank": rank,
            "default": 0,
        }
        self.output["theories"].append(t)
        if theoryType=="Utility":
            self.output["theories"][x]["Heuristic"] = [0 for _ in range(self.output["total_states"])]

    def getPolicyOutcomes(self, pi_name):
        self.outcomes[pi_name] = []
        self.probs[pi_name] = []
        theStack = [(0, 0, [0] * self.TheoryCount, 1)] # StateID, depth, wealth, prob
        while (len(theStack) > 0):
            state, depth, wealth, prob = theStack.pop()
            if (depth==self.horizon or len(self.stateTransitions[state].keys())==0):
                self.outcomes[pi_name].append(wealth)
                self.probs[pi_name].append(prob)
                continue
            for a, transitions in self.stateTransitions[state].items():
                for tr in transitions:
                    scr = tr[1]
                    d = depth+1
                    newWealth = [wealth[i] + tr[i + 2] for i in range(self.TheoryCount)]
                    p = tr[0] * prob
                    theStack.insert(0, (scr, d, newWealth, p))

    def findNonAccept(self):
        for i in range(len(self.Policies)):
            self.getPolicyOutcomes(self.intToCapStr(i))
        non_accept = [0 for _ in range(len(self.Policies))]
        for i in range(self.TheoryCount):
            non_accept = [a + b for a, b in zip(non_accept, self.findNonAcceptForTheory(i))] 
        return non_accept

    def findNonAcceptForTheory(self, theoryID) -> list:
        # Determine max estimate policy
        maxPolicy = 0
        for piIdx in range(len(self.Policies)):
            if (self.Policies[piIdx][1][theoryID] > self.Policies[maxPolicy][1][theoryID]):
                maxPolicy = piIdx
        
        # Determine max outcome in policy
        maxPolicyName = self.intToCapStr(maxPolicy)
        maxValue = self.outcomes[maxPolicyName][theoryID][0]
        for outValue in self.outcomes[maxPolicyName]:
            if outValue[theoryID] > maxValue:
                maxValue = outValue[theoryID]
        
        non_accept = [0 for _ in range(len(self.Policies))]
        for piIdx in range(len(self.Policies)):
            if (piIdx==maxPolicy):
                continue
            for outValueIdx in range(len(self.outcomes[self.intToCapStr(piIdx)])):
                if self.outcomes[self.intToCapStr(piIdx)][outValueIdx][theoryID] < maxValue:
                    non_accept[piIdx] += self.probs[self.intToCapStr(piIdx)][outValueIdx]
        return non_accept

    def computeProbs(self):
        changed = True
        while changed:
            changed = False
            new_p = p.copy()
            for s in range(num_states):
                if policy_name in self.stateTransitions[s]:
                    for trans in self.stateTransitions[s][policy_name]:
                        prob, succ = trans[0], trans[1]
                        new_p[succ] += p[s] * prob
            if any(abs(new_p[i] - p[i]) > 1e-8 for i in range(num_states)):
                changed = True
            p = new_p

    def compute_visitation_probs(self, policy_name):
        p = [0.0] * len(self.stateTransitions)
        p[0] = 1.0  # Start at state 0

        visited = [False] * len(self.stateTransitions)
        stack = [0]# Current state
        while len(stack)>0:
            s = stack.pop()
            visited[s] = True
            if policy_name not in self.stateTransitions[s]:
                continue
            for trans in self.stateTransitions[s][policy_name]:
                prob, succ = trans[0], trans[1]
                p[succ] += p[s] * prob
                if not visited[succ]:
                    stack.append(succ)
        return p
         
    def setAllRewards(self, policyExpectations:dict, minUtility:float, maxUtility:float):
        """
        Args:
            policyExpectations (dict): {"A": [exp_1, exp_2, ..., exp_n]}
        """
        x = 0
        for a, ex in policyExpectations.items():
            self.Policies.append([x, ex])
            x += 1

        totalPolicies = len(policyExpectations)
        totalStates = len(self.stateTransitions)
        totalTheories = len(policyExpectations["A"])
        totalVariables = totalStates * totalTheories

        # Random weights to create noise.
        # 0 is feasibility; 1 is minimisation.
        #coeff = [random.uniform(0.3, 0.7) for _ in range(totalVariables)]
        coeff = [0.0] * totalVariables

        equalConstraints = []
        targets = []
        for piName, exp in policyExpectations.items():
            probs = self.compute_visitation_probs(piName)
            for i in range(totalTheories):
                row = [0.0] * totalVariables
                for s in range(totalStates):
                    row[i * totalStates + s] = probs[s]
                equalConstraints.append(row)
                targets.append(exp[i])
        
        # Randomised bounds between -10 and 10
        #b = [(round(random.uniform(-15, -7), 3), round(random.uniform(7, 15), 3)) for _ in range(totalVariables)]
        b = [(-50, 50)] * totalVariables
        res = linprog(coeff, A_eq=equalConstraints, b_eq=targets, bounds=b, method='highs')
        if not res.success:
            raise ValueError(f"LP infeasible: {res.message}")

        utils = res.x.reshape((totalTheories, totalStates))

        for sId in range(len(self.stateTransitions)):
            for a in self.stateTransitions[sId]:
                for t in self.stateTransitions[sId][a]:
                    for i in range(self.TheoryCount):
                        t.append(utils[i][sId])

    def build(self, horizon=5, maxBranch=3, theories=3):
        self.TheoryCount=theories
        self.horizon=horizon
        self.maxBranchFactor=maxBranch
        self.stateTransitions = [{}]
        self.stateTimes = [0]
        self.outcomes = {}
        self.probs = {}
        self.depthMap={0:[0]}
        
        # Fills in state-transitions
        self.generatePolicy()
        self.generatePolicy()
        self.generatePolicy()
        self.setAllRewards({"A": [1,1,1], "B": [2,2,2], "C": [1,2,3]}, -5, 5)

        
        self.output["horizon"] = horizon
        self.output["actions"] = [self.intToCapStr(i) for i in range(len(self.Policies))]# New Action for each policy
        self.output["total_states"] = len(self.stateTransitions)
        self.output["state_transitions"] = self.stateTransitions
        self.output["state_time"] = self.stateTimes

        self.addTheory("Util_1", "Utility", rank=0)
        self.addTheory("Util_2", "Utility", rank=0)
        self.addTheory("Util_3", "Utility", rank=0)
            

        na = self.findNonAccept()
        self.output["expectedPolicies"] = []
        for id, expectations in self.Policies:
            self.output["expectedPolicies"].append({
                "Index": id,
                "Name": self.intToCapStr(id),
                "ExpectedWorth": expectations,
                "Non-Accept": na[id]
            })

        return
        
    def intToCapStr(self, i) -> str:
        return chr(ord('A') + (i))
    
    def generatePolicy(self, policyName="N/A"):
        # stateTransition [0 : {"Action" : [[Probability, successor, Worth 1, Worth 2...], [P, scr, w1, w2...]] } ] 

        has_used_new_action = False
        # Create policy
        # Stack contains tuples of (state, depth)
        theStack = [(0, 0)]
        while (len(theStack) > 0):
            currState, depth = theStack.pop()
            if (depth==self.horizon):
                continue

            pickedAction = self.chooseAction(currentActions=list(self.stateTransitions[currState].keys()), depth=depth, has_used_new_action=has_used_new_action)

            if (pickedAction=="new"):
                # new action
                has_used_new_action=True
                self.stateTransitions[currState][policyName] = []
                scrs = self.numberOfSuccessors()
                probs = self.successorProbabilities(scrs)
                
                for p in probs:
                    nextState = 0
                    statesAddedByAction = 0
                    if True:#(self.isNewState(depth+1, statesAddedByAction)):
                        statesAddedByAction += 1
                        nextState = len(self.stateTransitions)
                        self.stateTransitions.append({})
                        self.stateTimes.append(depth+1)
                        self.depthMap.setdefault(depth+1, [])
                        self.depthMap[depth+1].append(nextState)
                    else:
                        # Randomly choose existing state.
                        nextState = random.choice(self.depthMap[depth+1])

                    self.stateTransitions[currState].setdefault(policyName, [])
                    self.stateTransitions[currState][policyName].append([p, nextState])
                    theStack.insert(0, (nextState, depth+1))
                    
            else:
                for p, scr, *worth in self.stateTransitions[currState][pickedAction]:
                    theStack.insert(0, (scr, depth+1))

    def chooseAction(self, currentActions, depth, has_used_new_action):
        r = random.uniform(0,1)
        chanceOfNew = depth+1 / self.horizon
        pickedAction = "new"
        if ((len(currentActions)>1 and r >= chanceOfNew)):
            pickedAction = currentActions[math.floor((r * chanceOfNew / len(currentActions)) / chanceOfNew)]

        # If at the bottom and still not used a new action, then force it now at the end.
        if (not has_used_new_action and depth==self.horizon-1):
            pickedAction = "new"
        return pickedAction
    
    def numberOfSuccessors(self):
        return random.choice(list(range(1, self.maxBranchFactor+1)))

    def successorProbabilities(self, totalSuccessors):
        p = [random.random() for i in range(0,totalSuccessors)]
        tempSum = sum(p)
        return [ i/tempSum for i in p ]

    # Semi-randomly chooses whether to add a new state or transition to an old one.  
    # More states at next depth, less likely to add new; More states added by action, less likely to add new
    def isNewState(self, depth, statesAddedByAction):
        if (depth+1 not in self.depthMap.keys()):
            return True
        chance = 0.5 / (statesAddedByAction*0.8 + 0.2*len(self.depthMap[depth+1]))
        return random.random() < chance

b = Builder()
b.build()
json_string = myJSON.custom_json_format(b.output)
with open("testfile.json", 'w') as file:
    file.write(json_string)
    #json.dump(output, file, indent=2, cls=cJE)
print('Written to `' + "testfile.json" + '`')




'''
JSON should be formatted approx:
{
    horizon: INT,
    total_states: INT,
    actions: [STRINGS],
    state_transitions: [
        {
            "ACTION_ONE": [[PROB, SCR, TH_1, TH_2...], [PROB, SCR, TH_1, TH_2, ...], ...],
            "ACTION_TWO": [[PROB, SCR, TH_1, TH_2...], [PROB, SCR, TH_1, TH_2, ...], ...]
        },
        {
            ...
        },
        ...
    ],
    theories: [
        { FOR TH_1
            "Name": STRING,
            "Rank": INT,
            "Type": "Utility",
            "Heuristic":[FOR EVERY STATE],
            "default": TH_1value
        },
        ...
    ]
    "expectedPolicies": [
        {
            "expected": [Th_1, Th_2, ...],
            "outcomes" : [[Th_1, Th_2, ...], [Th_1, Th_2, ...], ...],
            "outcome_probability" : [[Th_1, Th_2, ...], [Th_1, Th_2, ...], ...],
            "non_accept": DOUBLE,
        }
    
    ]


}

s
'''