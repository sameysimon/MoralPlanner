from EnvironmentBuilder.BaseMDP import MDP, MoralTheory

class Time(MoralTheory):
    def __init__(self):
        self.type='utility'
        self.rank=1
        self.tag='time'
        self.default = 0

    def judge(self, successor: MDP.Successor):
        return -1

    # Simple hamming distance heuristic.
    def StateHeuristic(self, state:MDP.State):
        minDist = 99999
        xy = state.props['xy']
        for g in state.props['goals']:
            dist = abs(g[0] - xy[0]) + abs(g[1] - xy[1])
            if (dist < minDist):
                minDist = dist

        return -1 * minDist


class LifeAndDeath(MoralTheory):
    def __init__(self):
        self.type='utility'
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
    
class ToSteal(MoralTheory):
    def __init__(self):
        self.type='absolute'
        self.rank=3
        self.tag='ToSteal'
        self.default = False

    def judge(self, successor: MDP.Successor):
        if (successor.action=='steal'):
            return True
        return False
    
    def StateHeuristic(self, state:MDP.State):
        return False
    
    
    
    
class HalLife(MoralTheory):
    def __init__(self):
        self.type='utility'
        self.rank=0
        self.tag='HalLife'
        self.default = 0

    def judge(self, successor: MDP.Successor):
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            return -10
        return 0
    def StateHeuristic(self, state:MDP.State):
        return 0
    
class CarlaLife(MoralTheory):
    def __init__(self):
        self.type='utility'
        self.rank=0
        self.tag='CarlaLife'
        self.default = 0

    def judge(self, successor: MDP.Successor):
        if (successor.sourceState.props['Carla_alive']==True and successor.targetState.props['Carla_alive']==False):
            return -10
        return 0
    def StateHeuristic(self, state:MDP.State):
        return 0