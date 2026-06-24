from EnvironmentBuilder.BaseMDP import MDP, Consideration, State, Successor

HAL_ARREST = -10

HAL_WIN = -10
CARLA_WIN = -7

HAL_DEFEAT = -10
CARLA_DEFEAT = -20

HAL_DIE = -100
CARLA_DIE = -100

HAL_PAYS_LOW = -1
HAL_PAYS_HIGH = -3

CARLA_SEARCHED = -1
CARLA_INTRUDED = -2



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
        self.ordinalLabels = {"0": "No violation", "-1": "Opportunistic Intent", "-2": "Trespass", "-3": "Burglary", "-4": "Robbery", "-5": "Violent Theft"}

    def judge(self, successor: Successor):
        if (successor.action=='search_outside' and successor.targetState.props["Hal_at"]=='Carla_house'):
            return -2
        if (successor.action=='search_outside'):
            return -1
        if (successor.action=='sneak_inside' and successor.targetState.props["Hal_at"]=='Carla_house'):
            return -2
        if (successor.action=='intimidate' and successor.targetState.props["Hal_has_insulin"]==True):
            return -3
        if (successor.action=='break_in' and successor.targetState.props["Hal_at"]=='Carla_house'):
            return -4
        if (successor.action == 'attack_Carla'):
            return -5
        return 0
    
    def StateHeuristic(self, state:State):
        return 0
    

class OrdinalNecessity(Consideration):
    # Default to maximising the worst (pessimist). Set optimalityType to 1 to optimise the best (optimist)
    def __init__(self, optimalityType=0, tag='Necessity', horizon=6):
        super().__init__()
        self.type='Ordinal'
        self.rank=0
        self.optimalityType=optimalityType
        self.tag=tag
        self.default = 0
        self.horizon = horizon
        self.ordinalLabels = {"0": "No violation", "-1": "Probable death", "-2": "Serious medical risk", "-3": "Significant hardship", "-4": "Convenience only", "-5": "Beyond necessity"}

    def judge(self, successor: Successor):
        # Only crimes have necessity of crime.
        if (successor.action == 'attack_Carla'):
            return -5
        
        if (not (successor.action in ['steal', 'break_in', 'sneak_inside', 'attack_Carla'])):
            return 0
        die_chance = ((successor.targetState.props['time']  + 1) ) / (self.horizon + 1)
        if (die_chance >= 0.5):
            # MORE THAN 50% DEATH
            if (successor.sourceState.props['time']>0):
                if (successor.sourceState.props['Carla_sold'] == 'refused_high'):
                    return -1
                if (successor.sourceState.props['Carla_sold'] == 'refused_low'):
                    return -2
                return -3
            else:
                return -1
        else:
            # LESS THAN 50% OF DEATH
            if (successor.sourceState.props['Carla_sold'] == 'refused_high'):
                return -2
            if (successor.sourceState.props['Carla_sold'] == 'refused_low'):
                return -3
            if (successor.sourceState.props['Carla_reply'] == 'refused_ask'):
                return -4
            return -5
        
    def StateHeuristic(self, state:State):
        return 0

#
# Hal's personal moral consideratinos
#
class HalLife(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='HalLife'
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            return HAL_DIE
        return 0
        
    def StateHeuristic(self, state:State):
        return 0   
class HalSmall(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='HalSmall'
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.targetState.props['Hal_arrested']==True and successor.sourceState.props['Hal_arrested']==False):
            return HAL_ARREST
        if (successor.action == 'attack_Carla' and successor.targetState.props['Carla_reply']=='refused_attack'):
            return HAL_DEFEAT
        if (successor.action == 'buy_high' and successor.targetState.props['Carla_reply']=='sold'):
            return HAL_PAYS_HIGH
        if (successor.action == 'buy_low' and successor.targetState.props['Carla_reply']=='sold'):
            return HAL_PAYS_LOW
        if (successor.action == 'buy_low'):
            return HAL_PAYS_LOW
        if (successor.action == 'buy_high'):
            return HAL_PAYS_HIGH
            
        elif (successor.action == 'attack_Carla'):
            return HAL_WIN
        return 0
        
    def StateHeuristic(self, state:State):
        return 0
    
class Hal(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='Hal'
        self.default = 0
        self.small = HalSmall()
        self.life = HalLife()

    def judge(self, successor: Successor):
        return self.small.judge(successor) + self.life.judge(successor)
        
    def StateHeuristic(self, state:State):
        return max(self.small.StateHeuristic(state), self.life.StateHeuristic(state))
    

#
# Carla's moral considerations
#
class CarlaSmall(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='CarlaSmall'
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.action == 'attack_Carla' and successor.targetState.props['Carla_reply']=='refused_attack'):
            return CARLA_WIN
        elif (successor.action == 'attack_Carla'):
            return CARLA_DEFEAT
        elif (successor.action=='search_outside'):
            return CARLA_SEARCHED
        elif (successor.targetState.props['Hal_at']=='Carla_house' and successor.action=='sneak_inside'):
            return CARLA_INTRUDED
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
        if (successor.sourceState.props['Carla_alive']==True and successor.targetState.props['Carla_alive']==False):
            return CARLA_DIE
        return 0

    def StateHeuristic(self, state:State):
        return 0
    
class Carla(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='Carla'
        self.default = 0
        self.carlaLife = CarlaLife()
        self.carlaSmall = CarlaSmall()

    def judge(self, successor: Successor):
        return self.carlaLife.judge(successor) + self.carlaSmall.judge(successor)

    def StateHeuristic(self, state:State):
        return max(self.carlaLife.StateHeuristic(state), self.carlaSmall.StateHeuristic(state))

# Hal and Carla combined stuff
class OverallUtility(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag='Overall'
        self.default = 0
        self.carla = Carla()
        self.hal = Hal()

    def judge(self, successor: Successor):
        return self.carla.judge(successor) + self.hal.judge(successor)

    def StateHeuristic(self, state:State):
        return max(self.hal.StateHeuristic(state), self.carla.StateHeuristic(state))
