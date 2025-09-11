from EnvironmentBuilder.BaseMDP import MDP, MoralTheory
    
class TheLaw(MoralTheory):
    def __init__(self, theoryid):
        self.type='Absolutism'
        self.rank=0
        self.tag='AbstractLaw' + str(theoryid)
        self.default = False

    def judge(self, successor: MDP.Successor):
        return successor.targetState.props['law']
    
    def StateHeuristic(self, state:MDP.State):
        return False
    
class Utility(MoralTheory):
    def __init__(self, theoryid):
        self.theoryid = theoryid
        self.type='Utility'
        self.rank=0
        self.tag='AbstractUtility' + str(theoryid)
        self.default = 0

    def judge(self, successor: MDP.Successor):
        return successor.targetState.props['utility' + str(self.theoryid)]
        
    def StateHeuristic(self, state:MDP.State):
        return 10 * (20 - state.props['time'])
    

