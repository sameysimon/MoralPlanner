from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP, Consideration, State, Successor, Theory
import math

class Utility(Consideration):
    def __init__(self, tag='1', componentOf=[]):
        super().__init__()
        self.compoenentOf = componentOf
        self.type='Utility'
        self.rank=0
        self.tag = f"{tag}_class_deaths" if tag!="crew" else "crew_deaths"
        self.default = 0

    def judge(self, successor: Successor):
        return successor.targetState.props[self.tag] - successor.sourceState.props[self.tag]
        
    def StateHeuristic(self, state:State):
        return 0


class Titanic(MDP):
    TheoryClasses = {
        "Default": Utility
    }
    defaultProps = {
        "time":0,
        "1_class_deaths":0,
        "3_class_deaths":0,
        "crew_deaths":0,
        "seen_iceberg": False,
        "hit_back": False,
        "hit_mid": False,
        "hit_front": False,
        "x-front": 0,
        "y-front": -3,
        "x-back": 0,
        "y-back": -4,
        "iceberg": [[0,0], [1,0]]
    }

    def __init__(self, Theories, Considerations, initialProps=None, horizon=5, **kwargs):
        super().__init__()
        if initialProps==None:
            initialProps=Titanic.defaultProps
        if not horizon==None:
            initialProps['horizon'] = horizon
        self.stateFactory(initialProps) # Create at least one initial state

        self.rules = [Titanic.SetSpeed,
                      Titanic.EncounterIceberg, 
                      Titanic.Navigate, 
                      Titanic.Hit, 
                      Titanic.CrewDeath,
                      Titanic.MidDeath, 
                      Titanic.BackDeath, 
                      Titanic.AdvanceTime]

        #self.CostTheory = Time()
        self.theorySetup(Theories, Considerations)

        #self.budget = budget
        self.MaxOutsideTime = 3
        self.horizon=horizon

    def isGoal(self, state):
        return state.props['y-back']==0

    def getActions(self, state) -> list:
        if state.props['time'] >= self.horizon:
            return []
        if (state.props['hit_front'] or (state.props['hit_mid'] or state.props['hit_back'])):
            return []
        if state.props['time']==0:
            return ['fastest', 'fast', 'slow']
        if state.props['seen_iceberg']:
            return ['left', 'straight', 'right']
        if state.props['y-back'] == 0:
            return 
        return []

    def SetSpeed(self, props, prob, action):
        if (action=='slow'):
            props['x-front'] = 0
            props['y-front'] = -3
            props['x-back'] = 0
            props['y-back'] = -4
            return [(props, prob)]
        if (action=='fast'):
            props['x-front'] = 0
            props['y-front'] = -2
            props['x-back'] = 0
            props['y-back'] = -3
            return [(props, prob)]
        if (action=='fastest'):
            props['x-front'] = 0
            props['y-front'] = -1
            props['x-back'] = 0
            props['y-back'] = -2
            return [(props, prob)]
        return [(props, prob)]
    
    def EncounterIceberg(self, props, prob, action):
        if (props['time']!=0):
            return [(props, prob)]
        o = []
        
        props_ = deepcopy(props)
        props_['seen_iceberg'] = True
        o.append((props_, prob*0.1))

        props_ = deepcopy(props)
        props_['seen_iceberg'] = False
        o.append((props_, prob*0.9))
        
        return o

    def Navigate(self, props, prob, action):
        if not props['seen_iceberg']:
            return [(props, prob)]
        
        pointingLeft = props['x-front'] < props['x-back']
        pointingRight = props['x-front'] > props['x-back']
        pointingForward = not (pointingLeft or pointingRight)

        props['y-front']+=1
        props['y-back']+=1

        def moveLogic(agentAction, actStr, pointingOpposite, pointingForward, pr):
            if (agentAction!=actStr):
                return []
            
            direction = 1
            if (agentAction=='left'):
                direction = -1
            
            if (pointingOpposite):
                logicOuts=[]
                # Turn does not work.
                logicOuts.append((pr, prob*0.5))
                p_ = deepcopy(pr) # align back to the front
                p_['x-back'] -= direction
                logicOuts.append((p_, prob*0.5))
                return logicOuts
            
            if (pointingForward):
                logicOuts=[]
                # turn does not work
                logicOuts.append((pr, prob*0.7))
                # turn does work
                p_ = deepcopy(pr) 
                pr['x-front'] += direction
                logicOuts.append((p_, prob*0.3))
                return logicOuts

            return []

        if (pointingRight and action=='left'):
            print()
        o = []    
        o.extend(moveLogic(action, 'left', pointingRight, pointingForward, props))
        o.extend(moveLogic(action, 'right', pointingLeft, pointingForward, props))

        if (len(o)==0):
            return [(props, prob)]
        else:
            return o
    
    def Hit(self, props, prob, action):
        if not props['seen_iceberg']:
            return [(props, prob)]

        hit_front = False
        hit_back = False
        for [x,y] in props['iceberg']:
            if (props['x-front']==x and props['y-front']==y):
                hit_front=True

            if (props['x-back']==x and props['y-back']==y):
                hit_back=True
        

        if (not (hit_back or hit_front)):
            return [(props, prob)]
        
        props_ = deepcopy(props)
        props_['hit_front'] = hit_front
        props_['hit_back'] = hit_back

        o = []
        if (hit_back and hit_front):
            props_['hit_mid'] = True
            o.append((props_, prob))
            return o

        # Some probability of mid if only back is hit

        return [(props_, prob)]

    def CrewDeath(self, props, prob, action):
        if (props['hit_front']):
            props['crew_deaths'] = -200
        return [(props, prob)]

    def MidDeath(self, props, prob, action):
        if (props['hit_mid']):
            props['1_class_deaths'] -= math.floor(324/2)
            props['3_class_deaths'] -= math.floor(709/2)
            
        return [(props, prob)]

    def BackDeath(self, props, prob, action):
        if (props['hit_back']):
            props['1_class_deaths'] -= math.ceil(324/2)
            props['3_class_deaths'] -= math.ceil(709/2)
        return [(props, prob)]
            
    # time advances each transition
    def AdvanceTime(self, props, prob, action):
        props_ = deepcopy(props)
        props_["time"] = props_["time"] + 1
        return [(props_, prob)]


   
    def stateString(self, state) -> str:
        return str(state.props)
        
    def theorySetup(self, theories, considerations):
        for t in theories:
            mc = Theory()
            mc.name = t["Name"]
            mc.rank = t["Rank"]
            mc.type = t["Type"]
            self.Theories.append(mc)

        mc = 0
        for c in considerations:
            tag = c["Type"]
            if tag in ['1', '3', 'crew']:
                mc = Utility(tag=tag, componentOf=c["Component_of"])
            
            mc.componentOf=c["Component_of"]
            self.Considerations.append(mc)