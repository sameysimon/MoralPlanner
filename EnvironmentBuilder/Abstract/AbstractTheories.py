from EnvironmentBuilder.BaseMDP import MDP, Consideration, State, Successor
    
class TheLaw(Consideration):
    def __init__(self, theoryid):
        super().__init__()
        self.type='Absolutism'
        self.rank=0
        self.tag='AbstractLaw' + str(theoryid)
        self.default = False

    def judge(self, successor: Successor):
        return successor.targetState.props['law']
    
    def StateHeuristic(self, state:State):
        return False
    
class Utility(Consideration):
    def __init__(self, theoryid):
        super().__init__()
        self.theoryid = theoryid
        self.type='Utility'
        self.rank=0
        self.tag='AbstractUtility' + str(theoryid)
        self.default = 0

    def judge(self, successor: Successor):
        return successor.targetState.props['utility' + str(self.theoryid)]
        
    def StateHeuristic(self, state:State):
        return 10 * (20 - state.props['time'])
    
class DefaultMaximin(Consideration):
    def __init__(self, tag='Utility'):
        super().__init__()
        self.type='Maximin'
        self.componentOf='Rawls'# or another name
        self.rank=0
        self.tag=tag
        self.default = 0

    def judge(self, successor: Successor):
        return 0
        
    def StateHeuristic(self, state:State):
        return 0
