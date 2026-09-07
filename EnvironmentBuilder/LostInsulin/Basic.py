from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP, Theory, Successor, Consideration, State
import random

class Cost(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Cost'
        self.rank=0
        self.tag="Cost"
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.targetState.props['Stolen']):
            return 0
        return -1
    
    def StateHeuristic(self, state:State):
        return 0

class HalLife(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag="Hal's well-being"
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.sourceState.props['Hal_alive']==True and successor.targetState.props['Hal_alive']==False):
            return -10
        return 0
    def StateHeuristic(self, state:State):
        return 0
    
class CarlaLife(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag="Carla's well-being"
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.sourceState.props['Carla_alive']==True and successor.targetState.props['Carla_alive']==False):
            return -10
        return 0

    def StateHeuristic(self, state:State):
        return 0
    
class ToSteal(Consideration):
    def __init__(self):
        self.type='Absolutism'
        self.rank=3
        self.tag="Has stolen"
        self.default = False

    def judge(self, successor: Successor):
        if (successor.action=='steal'):
            return True
        return False
    
    def StateHeuristic(self, state:State):
        return False



class Odds():
    STEAL_HC_LIVE=0.6
    STEAL_H_DIE=0.15
    STEAL_C_DIE=0.15
    STEAL_HC_DIE=0.1
    HAL_LIVES=0.6


class BasicLostInsulin(MDP):

    def __init__(self, Theories, Considerations, InitialProps=None, Horizon=2, **kwargs):
        super().__init__()
        if InitialProps==None:
            InitialProps=BasicLostInsulin.defaultProps
        if not Horizon==None:
            InitialProps['horizon'] = Horizon
        self.horizon=Horizon
        
        self.stateFactory(InitialProps) # Create at least one initial state
        self.rules = [BasicLostInsulin.ToSteal, BasicLostInsulin.DeathChance, BasicLostInsulin.NextTime] 
        self.Theories = []
        self.theorySetup(Theories, Considerations)
        self.isNonMoral = False

    defaultProps = {
            'time':0,
            'Stolen': False,
            'Hal_alive': True,
            'Carla_alive': True,
        }

    def isGoal(self, state):
        # Non-moral goal
        return False


    def getActions(self, state):
        if (state.props['time']>=self.horizon):
            return []
        if (state.props['Hal_alive']==False or state.props['Stolen']):
            return ['wait']
        
        return ['steal', 'wait']

    # Second choices: compensate low, high, nothing, or leave.
    def ToSteal(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        if (action!='steal'):
            return [(props, prob)]
        
        outcomes = []
        p_ = deepcopy(props)
        p_['Hal_alive'] = True
        p_['Carla_alive'] = True
        p_['Stolen'] = True
        outcomes.append((p_, prob*Odds.STEAL_HC_LIVE))
        p_ = deepcopy(props)
        p_['Hal_alive'] = False
        p_['Carla_alive'] = True
        p_['Stolen'] = True
        outcomes.append((p_, prob*Odds.STEAL_H_DIE))
        p_ = deepcopy(props)
        p_['Hal_alive'] = True
        p_['Carla_alive'] = False
        p_['Stolen'] = True
        outcomes.append((p_, prob*Odds.STEAL_C_DIE))
        p_ = deepcopy(props)
        p_['Hal_alive'] = False
        p_['Carla_alive'] = False
        p_['Stolen'] = True
        outcomes.append((p_, prob*Odds.STEAL_HC_DIE))

        return outcomes

    def DeathChance(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        if (props['Stolen']):
            return [(props, prob)]
        o = []
        p_ = deepcopy(props)
        p_['Hal_alive'] = False
        o.append((p_, prob*(1-Odds.HAL_LIVES)))

        p_ = deepcopy(props)
        p_['Hal_alive'] = True
        o.append((p_, prob*(Odds.HAL_LIVES)))
        return o
    
    def NextTime(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        props['time'] += 1
        return [(props, prob)]
        


    # Setup stuff.
    def stateString(self, state) -> str:
        return str(state.props)
        
    def theorySetup(self, theories, considerations):
        for t in theories:
            mc = Theory()
            mc.name = t["Name"]
            mc.rank = t["Rank"]
            mc.type = t["Type"]
            self.Theories.append(mc)
        
        rank = 0
        mc = 0
        for c in considerations:
            tag = c["Type"]
            if 'ToSteal'==tag:
                mc = ToSteal()
            elif 'HalLife'==tag:
                mc = HalLife()
            elif 'CarlaLife'==tag:
                mc = CarlaLife()
            elif 'Cost'==tag:
                mc = Cost()
            else:
                raise Exception(f"Moral theory with tag {tag} at rank {str(rank)} invalid.")
            mc.componentOf=c["Component_of"]
            self.Considerations.append(mc)

    def optionsString():
        s = "Initial property options are \n{`xy`: [int], `max_xy`: [int], `walls`:`max_xy`: [[int,int]], `playgrounds`:`max_xy`: [[int,int]], `goals`:`max_xy`: [[int,int]]} \n"
        return s + "Theory options are `time`, `avoid_playgrounds`, `avoid_checkpoints`."