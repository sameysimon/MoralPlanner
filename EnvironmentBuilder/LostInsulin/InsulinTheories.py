from EnvironmentBuilder.BaseMDP import MDP, Consideration, State, Successor

class Time(Consideration):
    def __init__(self, horizon_):
        self.type='Cost'
        self.rank=1
        self.tag='Cost'
        self.default = 0
        self.horizon = horizon_

    def judge(self, successor: Successor):
        if successor.targetState.props['Hal_has_insulin']:
            return 0
        return -1
    
    def StateHeuristic(self, state:State):
        return 0
  

# Stealing theories

class ToSteal(Consideration):
    def __init__(self):
        self.type='Absolutism'
        self.rank=3
        self.tag='ToSteal'
        self.default = False

    def judge(self, successor: Successor):
        if (successor.action=='steal'):
            return True
        return False
    
    def StateHeuristic(self, state:State):
        return False
    
class StealWithComp(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Absolutism'
        self.rank=3
        self.tag='StealWithComp'
        self.default = False

    def judge(self, successor: Successor):
        if (successor.action=='steal' and not (successor.sourceState.props['Carla_compensated'])):
            return True

        return False
    
    def StateHeuristic(self, state:State):
        return False
    

# Hal and Carla as individuals

class HalLife(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='HalLife'
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            return -10
        if (successor.targetState.props['Hal_arrested']==True and successor.sourceState.props['Hal_arrested']==False):
            return -1
        return 0
        
    def StateHeuristic(self, state:State):
        return 0
    
class CarlaLife(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='CarlaLife'
        self.default = 0

    def judge(self, successor: Successor):
        u = 0
        if (successor.sourceState.props['Hal_at']=="Carla_house"):
            u -= 1 
        if (successor.sourceState.props['Carla_alive']==True and successor.targetState.props['Carla_alive']==False):
            u -= 10
        return u

    def StateHeuristic(self, state:State):
        return 0


# Hal and Carla combined stuff

class OverallUtility(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='Overall'
        self.default = 0

    def judge(self, successor: Successor):
        u =0
        if (successor.sourceState.props['Carla_alive']==True and successor.targetState.props['Carla_alive']==False):
            u -= 10
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            u -= 10
        if (successor.targetState.props['Hal_arrested']==True and successor.sourceState.props['Hal_arrested']==False):
            u -= 1
        
        return u

    def StateHeuristic(self, state:State):
        return 0

class LifeAndDeath(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='LifeAndDeath'
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            return -10
        if (successor.sourceState.props['Carla_alive']==True and successor.targetState.props['Carla_alive']==False):
            return -10
        return 0
    
    def StateHeuristic(self, state:State):
        return 0