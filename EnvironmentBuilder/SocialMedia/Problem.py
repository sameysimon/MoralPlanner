from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP
from EnvironmentBuilder.GridWorld.DroneTheories import *
import random


class SocialMediaProblem(MDP):

    def __init__(self,**kwargs) -> None:
        kwargs.setdefault('horizon', 4)
        kwargs.setdefault('budget', 8)
        kwargs.setdefault('theoryClasses', [['time']])
        kwargs.setdefault('initialProps', None)

        super().__init__()
        if kwargs['initialProps']==None:
            initialProps=SocialMediaProblem.defaultProps
        self.horizon=kwargs['horizon']
        self.budget=kwargs['budget']

        self.stateFactory(initialProps) # Create at least one initial state
        self.rules = [] 
        # self.CostTheory = Time()
        self.Theories = []
        self.theorySetup(kwargs['theoryClasses'])
    
    defaultProps = {
            "attention": 5,
            "user_preference": "LEFT",
            "time": 0
        }

    def getActions(self, state):
        if (state.props['terminated']):
            return []
        
        return ['LEFT', 'CENTRE', 'RIGHT', 'ADVERT']

    def isGoal(self, state):
        xy = state.props['xy']
        return any(xy==g for g in state.props['goals'])
            
    def checkMove(props, newX, newY):
        if (0 > newX or newX >=  props['max_xy'][0]):
            return False
        if (0 > newY or newY >= props['max_xy'][1]):
            return False
        if ([newX,newY] in props['walls']):
            return False
        return True


    # standard move action
    def ActionHandler(self, props, prob, action):
        props_ = deepcopy(props)
        props_['time']+=1
        if (props['user_preference']==action and props['attention']<5):
            props_['attention']+=1
        elif (props['user_preference']!=action and props['attention']>0):
            props_['attention']-=1
        
        return [(props_, prob*1)]
        

    def terminateRule(self, props, prob, action):
        props_ = deepcopy(props)
        if (props['xy'] in props['goals']):
            props_['terminated'] = True
            return [(props_,prob*1)]
        return [(props_,prob*1)]



    # Setup stuff.
    def stateString(self, state) -> str:
        return ""

    def theorySetup(self, theoryClasses):
        rank = 0
        mt = 0
        for theoryGroup in theoryClasses:
            for tag in theoryGroup:
                if 'Cost'==tag:
                    mt = Time()
                elif 'MoralTime'==tag:
                    mt = MoralTime()
                elif 'avoid_playgrounds'==tag:
                    mt = AvoidPlaygrounds()
                elif  'avoid_checkpoints'==tag:
                    mt = AvoidCheckpoints()
                else:
                    raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
                mt.rank = rank
                self.Theories.append(mt)
            rank+=1
    def optionsString():
        s = "Initial property options are \n{`xy`: [int], `max_xy`: [int], `walls`:`max_xy`: [[int,int]], `playgrounds`:`max_xy`: [[int,int]], `goals`:`max_xy`: [[int,int]]} \n"
        return s + "Theory options are `time`, `avoid_playgrounds`, `avoid_checkpoints`."