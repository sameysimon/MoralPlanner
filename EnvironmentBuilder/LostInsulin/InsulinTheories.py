from EnvironmentBuilder.BaseMDP import MDP, Consideration, State, Successor

class Time(Consideration):
    def __init__(self, horizon_, budget_):
        self.type='Cost'
        self.rank=1
        self.tag='Cost'
        self.default = 0
        self.horizon = horizon_
        self.budget = budget_

    def judge(self, successor: Successor):
        if (successor.targetState.props['Hal_has_insulin']):
            return 0
        if not successor.targetState.props['Hal_alive']:
            return -self.budget
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
    
class Trespass(Consideration):
    def __init__(self):
        self.type='Absolutism'
        self.rank=3
        self.tag='Trespass'
        self.default = False

    def judge(self, successor: Successor):
        if (successor.targetState.props['Hal_at']=='Carla_house'):
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


class Relationship(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='Relationship'
        self.default = 0

    def judge(self, successor: Successor):
        r = 0
        # betrayal/lied
        if (successor.targetState.props['Carla_reply']=='refused' and successor.action=='steal'):
            r += -10 * successor.targetState.props['friendship']
        
        # stealing without betrayal/lie
        if (successor.targetState.props['Carla_reply']=='na' and successor.action=='steal'):
            r += -7 * successor.targetState.props['friendship']
        
        # compensation 
        if (successor.targetState.props['Carla_compensated'] and successor.action=='steal'):
            r += 1
        
        if (successor.targetState.props['Carla_compensated']):
            r += 1



        return r
        
    def StateHeuristic(self, state:State):
        return 0


class OrdinalLaw(Consideration):
    # Default to maximising the worst (pessimist). Set optimalityType to 1 to optimise the best (optimist)
    def __init__(self, optimalityType=0, tag='Legality'):
        super().__init__()
        self.type='Ordinal'
        self.rank=0
        self.optimalityType=optimalityType
        self.tag=tag
        self.default = 0
        self.ordinalLabels = {"0": "No violation", "-1": "Opportunistic Intent", "-2": "Trespass", "-3": "Robbery", "-4": "Burglary", "-5": "Violent Theft"}

    def judge(self, successor: Successor):
        if (successor.action=='search_outside' and successor.targetState.props["Hal_at"]=='Carla_house'):
            return -2
        if (successor.action=='search_outside'):
            return -1
        if (successor.action=='intimidate' and successor.targetState.props["Hal_has_insulin"]==True):
            return -3
        if (successor.targetState.props["Entry_status"]=='broken_in'):
            return -4
        if (successor.action == 'attack_Carla'):
            return -5
        return 0
    
    def StateHeuristic(self, state:State):
        return 0
    

class OrdinalNecessity(Consideration):
    # Default to maximising the worst (pessimist). Set optimalityType to 1 to optimise the best (optimist)
    def __init__(self, optimalityType=0, tag='Necessity'):
        super().__init__()
        self.type='Ordinal'
        self.rank=0
        self.optimalityType=optimalityType
        self.tag=tag
        self.default = 0
        self.ordinalLabels = {"0": "No violation", "-1": "Probable death", "-2": "Serious medical risk", "-3": "Significant hardship", "-5": "Discomfort", "-6": "Convenience only"}

    def judge(self, successor: Successor):
        # Only crimes have necessity of crime.
        if (not (successor.action in ['steal', 'give_low', 'give_high', 'break_in', 'attack_Carla'])):
            return 0
        x = successor.targetState.props['time'] * -1
        x = -6 if x < -6 else x
        return x
        
    def StateHeuristic(self, state:State):
        return 0



# Hal and Carla as individuals

class HalLife(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='HalLife'
        self.default = 0

    def judge(self, successor: Successor):
        u = 0
        if (successor.targetState.props['Hal_arrested']==True and successor.sourceState.props['Hal_arrested']==False):
            return -1
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            return -10
        return u
        
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
        if (successor.action == 'attack_Carla'):
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