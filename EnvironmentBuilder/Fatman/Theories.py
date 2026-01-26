from EnvironmentBuilder.BaseMDP import MDP, Consideration



class Utility(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='Act-Utilitarianism'
        self.default = 0

    def judge(self, successor: Successor):
        u = 0
        if (successor.sourceState.props['group_alive']==True and successor.targetState.props['group_alive']==False):
            u -= 5
        if (successor.sourceState.props['person_alive']==True and successor.targetState.props['person_alive']==False):
            u -= 1
        if (successor.sourceState.props['fatman_alive']==True and successor.targetState.props['fatman_alive']==False):
            u -= 1
        
        return u

    def StateHeuristic(self, state:State):
        return 0
    

class PoDE(Consideration):
    def __init__(self):
        super().__init__()
        self.type='PoDE'
        self.rank=0
        self.tag='Principle of Double Effect'
        self.default = ([], 0, True, 0)

    def getFacts(self):
        return ["group_died", "person_died", "fatman_died"]

    def judge(self, successor: Successor):
        facts = []
        moralGoalP = 0
        isActionGood = True

        if (successor.sourceState.props['group_alive']==True and successor.targetState.props['group_alive']==False):
            facts.append("group_died")
        if (successor.sourceState.props['person_alive']==True and successor.targetState.props['person_alive']==False):
            facts.append("person_died")
        if (successor.sourceState.props['fatman_alive']==True and successor.targetState.props['fatman_alive']==False):
            facts.append("fatman_died")

        if (self.IsMoralGoal(successor.targetState)):
            moralGoalP = 1

        # Action does not exist, here as an example
        if (successor.action=="EVIL"):
            isActionGood = False

        # Steal Utilitarianism's utility function
        utils = Utility.judge(self, successor)

        return (facts, moralGoalP, isActionGood, utils)
        

    def StateHeuristic(self, state:State):
        return ([], 0, True, 0)
    
    def IsMoralGoal(self, state:State):
        return (state.props["group_alive"]) and (state.props["group_alive"] and state.props["fatman_alive"])
    