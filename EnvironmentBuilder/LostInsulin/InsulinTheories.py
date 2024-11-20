from EnvironmentBuilder.BaseMDP import MDP, MoralTheory

class Time(MoralTheory):
    def __init__(self):
        self.type='Cost'
        self.rank=1
        self.tag='time'
        self.default = 0

    def judge(self, successor: MDP.Successor):
        if successor.targetState.props['Hal_has_insulin']:
            return 0
        return -1
    
    def StateHeuristic(self, state:MDP.State):
        return 0

    
class ToSteal(MoralTheory):
    def __init__(self):
        self.type='Absolutism'
        self.rank=3
        self.tag='ToSteal'
        self.default = False

    def judge(self, successor: MDP.Successor):
        if (successor.action=='steal'):
            return True
        return False
    
    def StateHeuristic(self, state:MDP.State):
        return False
    
class StealWithComp(MoralTheory):
    def __init__(self):
        self.type='Absolutism'
        self.rank=3
        self.tag='ToSteal'
        self.default = False

    def judge(self, successor: MDP.Successor):
        if (successor.action=='steal' and not (successor.sourceState.props['Carla_compensated'])):
            return True

        return False
    
    def StateHeuristic(self, state:MDP.State):
        return False
    
class HalLife(MoralTheory):
    def __init__(self):
        self.type='Utility'
        self.rank=0
        self.tag='HalLife'
        self.default = 0

    def judge(self, successor: MDP.Successor):
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            return -10
        if (successor.targetState.props['Hal_arrested']==True and successor.sourceState.props['Hal_arrested']==False):
            return -1
        return 0
        
    def StateHeuristic(self, state:MDP.State):
        if (state.props['Hal_neutral'] or state.props['Hal_arrested']):
            return -0.4
        return 0
    
class CarlaLife(MoralTheory):
    def __init__(self):
        self.type='Utility'
        self.rank=0
        self.tag='CarlaLife'
        self.default = 0

    def judge(self, successor: MDP.Successor):
        if (successor.sourceState.props['Carla_alive']==True and successor.targetState.props['Carla_alive']==False):
            return -10
        return 0

    def StateHeuristic(self, state:MDP.State):
        if (not state.props['Carla_has_insulin']):
            return -0.1
        return 0


class LifeAndDeath(MoralTheory):
    def __init__(self):
        self.type='Utility'
        self.rank=0
        self.tag='LifeAndDeath'
        self.default = 0

    def judge(self, successor: MDP.Successor):
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            return -10
        if (successor.sourceState.props['Carla_alive']==True and successor.targetState.props['Carla_alive']==False):
            return -10
        return 0
    
    def StateHeuristic(self, state:MDP.State):
        return 0