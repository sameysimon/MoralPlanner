from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP
from EnvironmentBuilder.GridWorld.DroneTheories import *
import random


class GridWorldProblem(MDP):

    def __init__(self,**kwargs) -> None:
        kwargs.setdefault('horizon', 4)
        kwargs.setdefault('budget', 8)
        kwargs.setdefault('theoryClasses', [['time']])
        kwargs.setdefault('initialProps', None)

        super().__init__()
        if kwargs['initialProps']==None:
            initialProps=GridWorldProblem.defaultProps
        self.horizon=kwargs['horizon']
        self.budget=kwargs['budget']

        self.stateFactory(initialProps) # Create at least one initial state
        self.rules = [GridWorldProblem.Move, GridWorldProblem.terminateRule] 
        self.CostTheory = Time()
        self.Theories = []
        self.theorySetup(kwargs['theoryClasses'])
        

    #moveMap = {'up': [0,1], 'down':[0,-1], 'left': [-1,0], 'right':[1,0]}
    moveMap = {'up': [0,1], 'right':[1,0]}
    defaultProps = {
            'xy':[0,0],
            'max_xy':[4,4],
            'walls':[[2,1],[2,2]],
            'playgrounds':[[0,3]],
            'checkpoints':[],
            'goals':[[3,3]],
            'terminated':False
        }

    def getActions(self, state):
        if (state.props['terminated']):
            return []
        if (state.id==9):
            a=0
        a = []
        newX = GridWorldProblem.moveMap['up'][0] + state.props['xy'][0]
        newY = GridWorldProblem.moveMap['up'][1] + state.props['xy'][1]
        if (GridWorldProblem.checkMove(state.props, newX, newY)):
            a.append('up')
        newX = GridWorldProblem.moveMap['right'][0] + state.props['xy'][0]
        newY = GridWorldProblem.moveMap['right'][1] + state.props['xy'][1]
        if (GridWorldProblem.checkMove(state.props, newX, newY)):
            a.append('right')
        return ['up', 'right']

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
    def Move(self, props, prob, action):
        outcomes = []
        tempActions = deepcopy(list(GridWorldProblem.moveMap.keys()))
        tempActions.remove(action)
        mainProb = 0.7
        #if (action=='left' or action=='right'):
        #    mainProb = 0.8
        
        # Greatest chance of normal move.
        success_props = deepcopy(props)
        x, y = success_props['xy'][0], success_props['xy'][1]
        deltaX, deltaY = GridWorldProblem.moveMap[action][0], GridWorldProblem.moveMap[action][1]
        newX, newY = x + deltaX, y + deltaY
        if (GridWorldProblem.checkMove(props,newX,newY)):
            success_props['xy'] = [newX, newY]# only if can move, change coords. Otherwise, coords stay the same.
            outcomes.append((success_props, prob*mainProb))
        if (not GridWorldProblem.checkMove(props,newX,newY)):
            outcomes.append((success_props, prob))
            return outcomes

        # Try finding a wrong action.
        probRemaining=1-mainProb
        
        new_props = []
        for a in tempActions:
            x, y = props['xy'][0], props['xy'][1]
            deltaX, deltaY = GridWorldProblem.moveMap[a][0], GridWorldProblem.moveMap[a][1]
            newX, newY = x + deltaX, y + deltaY
            if (GridWorldProblem.checkMove(props,newX,newY)):
                p = deepcopy(props)
                p['xy']= [newX, newY]
                new_props.append(p)
                break
        
        for p in new_props:
            outcomes.append((p, probRemaining/len(new_props)))

        if (len(outcomes)==1):
            #failed to add wrong action to list, change probability to 1
            outcomes = [(success_props, prob*1)]

        return outcomes

    def terminateRule(self, props, prob, action):
        props_ = deepcopy(props)
        if (props['xy'] in props['goals']):
            props_['terminated'] = True
            return [(props_,prob*1)]
        return [(props_,prob*1)]



    # Setup stuff.
    def stateString(self, state) -> str:
        p = state.props
        s="ID={id} week={week}\n".format(week=p['week'], id=state.id)
        for idx in range(0, state.props['totalStudents']):
            if state.props['student']==idx:
                s+="*"
            s+="Student {i}: (grd:{grade}+{finChance},  str:{stress})\n".format(
                i=idx, 
                finChance=round(p['standardChances'][idx],2),
                grade=p['grades'][idx], 
                stress=p['stress'][idx])
        return s

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