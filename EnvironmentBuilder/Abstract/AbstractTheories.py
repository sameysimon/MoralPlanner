from EnvironmentBuilder.BaseMDP import MDP, MoralTheory
    
class TheLaw(MoralTheory):
    def __init__(self):
        self.type='Absolutism'
        self.rank=0
        self.tag='ToSteal'
        self.default = False

    def judge(self, successor: MDP.Successor):
        return successor.targetState.props['law']
    
    def StateHeuristic(self, state:MDP.State):
        return False
    
class Utility(MoralTheory):
    def __init__(self):
        self.type='Utility'
        self.rank=0
        self.tag='HalLife'
        self.default = 0

    def judge(self, successor: MDP.Successor):
        return successor.targetState.props['utility']
        
    def StateHeuristic(self, state:MDP.State):
        return 10 * (20 - state.props['time'])
    

