from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP, Consideration, Successor, State

class DefaultUtility(Consideration):
    def __init__(self, tag='Utility'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0

    def judge(self, successor: Successor):
        return 0
        
    def StateHeuristic(self, state:State):
        return 0


class Template(MDP):
    TheoryClasses = {
        "DefaultUtility": DefaultUtility
    }
    defaultProps = {"time":0}

    def __init__(self, initialProps=None, horizon=5, theoryClasses=[['utility']], **kwargs):
        super().__init__()
        if initialProps==None:
            initialProps=Template.defaultProps
            pass
        if not horizon==None:
            initialProps['horizon'] = horizon
        self.stateFactory(initialProps) # Create at least one initial state

        self.rules = [] 

        #self.CostTheory = Time()
        self.Theories = []
        self.theorySetup(theoryClasses)
        
        #self.budget = budget
        self.MaxOutsideTime = 3
        self.horizon=horizon


    def getActions(self, state) -> list:
        if state.props['time'] > self.horizon:
            return []
        pass

    # time advances each transition
    def AdvanceTime(self, props, prob, action):
        props_ = deepcopy(props)
        props_["time"] = props_["time"] + 1
        return [(props_, prob)]



     # Setup stuff.
    def stateString(self, state) -> str:
        return str(state.props)
        
    def theorySetup(self, theoryClasses):
        rank = 0
        mt = 0
        theoryid = 0
        for theoryGroup in theoryClasses:
            for tag in theoryGroup:
                if (not tag in Template.TheoryClasses.keys()):
                    raise Exception(f"No moral theory with tag '{tag}' for domain 'Template'.")
                mt = Template.TheoryClasses[tag]()
                mt.rank = rank
                self.Theories.append(mt)
                theoryid+=1
            rank+=1