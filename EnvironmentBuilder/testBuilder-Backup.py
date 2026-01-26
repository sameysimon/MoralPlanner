import random
import math
import CustomJSON as myJSON


class Builder:
    def __init__(self):
        self.output = {
            "theories": []
        }
        self.Policies = []
        self.defeatAdjList = []
        self.TheoryCount = 0
    




    def addTheory(self, name="Utilitarian", theoryType="Utility", rank=0):
        t = {
            "Name": name,
            "Type": theoryType,
            "Rank": rank,
            "default": 0
        }
        self.output["theories"].append(t)

    def findNonAccept(self):
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
        
        # Determine outcome in policy
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



        

    def build(self, horizon=5, maxBranch=3, theories=3):
        self.TheoryCount=theories
        self.horizon=horizon
        self.maxBranchFactor=maxBranch
        self.stateTransitions = [{}]
        self.stateTimes = [0]
        self.rootState = {}
        self.outcomes = {}
        self.probs = {}
        self.depthMap={0:0}
        
        # Fills in state-transitions
        self.generatePolicy([1,1,1])
        self.generatePolicy([2,2,2])
        self.generatePolicy([1,2,3])
        self.generatePolicy([3,3,3])

        
        self.output["horizon"] = horizon
        self.output["actions"] = [self.intToCapStr(i) for i in range(len(self.Policies))]# New Action for each policy
        self.output["total_states"] = len(self.stateTransitions)
        self.output["state_transitions"] = self.stateTransitions
        self.output["state_time"] = self.stateTimes

        for tIdx in range(self.TheoryCount):
            # TODO mechanism for other theories
            self.addTheory()
            self.output["theories"][tIdx]["Heuristic"] = [0 for _ in range(self.output["total_states"])]

        na = self.findNonAccept()
        self.output["expectedPolicies"] = []
        for id, expectations in self.Policies:
            self.output["expectedPolicies"].append({
                "Index": id,
                "Name": self.intToCapStr(id),
                "ExpectedWorth": expectations,
                "Non-Accept": na[id]
            })

     
    # Utilitarianism
    def getUtility(self, remainingUtility, probability, depth):
        if (depth==self.horizon-1):
            return remainingUtility / (probability + 0.0) # Ensures we expend last of utility at last step.
        return (random.uniform(-20, 20) / (self.horizon+0.0))
        
    def getRemainingUtility(self, remainingUtility, probability, util):
        return remainingUtility - (probability * util)

    # All theory getters
    def getValues(self, remainingValues, prob, depth):
        l=[]
        for i in remainingValues:
            l.append(self.getUtility(i, prob, depth))
            # TODO. Switch for different types of considerations
        return l        

    def getRemainingValues(self, remainingValues, prob, values):
        l = []
        for i in range(len(remainingValues)):
            l.append(self.getRemainingUtility(remainingValues[i], prob, values[i]))
            # TODO. Switch for different types of considerations
        return l

    def intToCapStr(self, i) -> str:
        return chr(ord('A') + (i))
    
    def generatePolicy(self, expectedWorth=[5,2,-3], policyName="N/A"):
        if policyName=="N/A":
            policyName = self.intToCapStr(len(self.Policies))
        self.Policies.append([len(self.Policies), expectedWorth])

        # EU = p[1] * u[1] + p[2] * u[2] + ...
        # EU - p[1] * u[1] = rest of EU
        # 5 - randProb * randUtil = rest of EU
        # stateTransition [0 : {"Action" : [[Probability, successor, Worth 1, Worth 2...], [P, scr, w1, w2...]] } ] 
        has_used_new_action = False
        # Create policy
        # Stack contains tuples of (state, depth, accruedWealth)
        theStack = [(0,0, [0 for _ in expectedWorth], 1.0, expectedWorth)]# TODO make it multi-objective
        while (len(theStack) > 0):
            currState, depth, wealth, probability, expWorth = theStack.pop()
            if (depth==self.horizon):
                self.outcomes.setdefault(policyName, [])
                self.probs.setdefault(policyName, [])
                self.outcomes[policyName].append(wealth)
                self.probs[policyName].append(probability)
                continue

            actions = list(self.stateTransitions[currState].keys())
            r = random.uniform(0,1)
            chanceOfNew = depth+1 / self.horizon
            pickedAction = "new"
            if ((len(actions)>1 and r >= chanceOfNew)):
                pickedAction = actions[math.floor((r * chanceOfNew / len(actions)) / chanceOfNew)]

            # If at the bottom and still not used a new action, then force it now at the end.
            if (not has_used_new_action and depth==self.horizon-1):
                pickedAction = "new"

            if (pickedAction=="new"):
                # new action
                if (not has_used_new_action):
                    self.rootState[policyName] = currState
                has_used_new_action=True
                self.stateTransitions[currState][policyName] = []
                branches = random.choice(list(range(1, self.maxBranchFactor+1)))
                probs = [random.random() for i in range(0,branches)]
                tempSum = sum(probs)
                probs = [ i/tempSum for i in probs ]
                for p in probs:
                    values = self.getValues(expWorth, p, depth)
                    nextState = len(self.stateTransitions)
                    tr = [p, nextState]
                    tr.extend(values)
                    self.stateTransitions[currState].setdefault(policyName, [])
                    self.stateTransitions[currState][policyName].append(tr)
                    probOfScr = probability * p

                    theStack.insert(0, (len(self.stateTransitions), depth+1, list(map(sum, zip(wealth,values))), probOfScr, self.getRemainingValues(expWorth, p, values)) )
                    self.stateTransitions.append({})
                    self.stateTimes.append(depth)
            else:
                for p, scr, *worth in self.stateTransitions[currState][pickedAction]:
                    theStack.insert(0, 
                                    (scr, depth+1, list(map(sum, zip(wealth,worth))), p, self.getRemainingValues(expWorth, p, worth) ))
                    

def set_constraint(self, policy_name:str, obj:int, operator:str, util:float, minProb:float):
    # First, check constraint
    prSatisfied = 0
    i=0
    for worths in self.outcomes[policy_name]:
        if (operator=="lt"):
            if worths[obj] < util:
                prSatisfied += self.probs[policy_name][i]
        else:
            if worths[obj] > util:
                prSatisfied += self.probs[policy_name][i]
        i+=1
    if (prSatisfied > minProb):
        # Satisfied!
        return

    # Second, modify policy to fit new outcomes
    theStack = [self.rootState[policy_name]]


    
    
b = Builder()
b.build()
json_string = myJSON.custom_json_format(b.output)
with open("testfile.json", 'w') as file:
    file.write(json_string)
    #json.dump(output, file, indent=2, cls=cJE)
print('Written to `' + "testfile.json" + '`')