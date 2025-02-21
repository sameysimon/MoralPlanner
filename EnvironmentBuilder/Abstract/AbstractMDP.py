from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP
from EnvironmentBuilder.Abstract.AbstractTheories import *
import numpy as np
import random


class AbstractMDP(MDP):

    def __init__(self, initialProps=None, horizon=20, budget=6, goalOdds=0, branchFactor=2, actionFactor=2, theoryClasses=[['utility', 'law']], **kwargs):
        super().__init__()
        random.seed(kwargs['seed'])
        np.random.seed(kwargs['seed'])
        if initialProps==None:
            initialProps=AbstractMDP.defaultProps
        if not horizon==None:
            initialProps['horizon'] = horizon
        self.stateFactory(initialProps) # Create at least one initial state
        self.rules = [AbstractMDP.Next] 
        #self.CostTheory = Time()
        self.Theories = []
        self.theorySetup(theoryClasses)
        
        self.budget = budget
        self.goalOdds = goalOdds
        self.isNonMoral = goalOdds>0

        self.horizon=horizon
        self.branchFactor=branchFactor
        self.actionFactor=actionFactor

        self.myActions = []
        for i in range(actionFactor):
            self.myActions.append('a_' + str(i))
    defaultProps = {
            'time':0,
            'utility':0,
            'law': False,
            'isGoal':False
        }


    def getActions(self, state):
        if not(state.props['time'] <= self.horizon-1):
            return []
        return self.myActions

    def isGoal(self, state):
        return state.props['isGoal']

    # Second choices: compensate low, high, nothing, or leave.
    def Next(self, props, prob, action):
        outs = []
        if not(props['time'] <= self.horizon-1):
            return outs
        probDist = np.random.dirichlet(np.ones(self.branchFactor), size=1)[0]
        for i in range(self.branchFactor):
            props_ = deepcopy(props)
            props_['time']+=1
            props_['utility'] = random.uniform(-10, 10)
            props_['law'] = random.random() > 0.5
            if (props_['time']>=self.horizon/2):
                props_['isGoal']=random.random()>self.goalOdds
            outs.append((props_, prob * probDist[i]))
        return outs



    # Setup stuff.
    def stateString(self, state) -> str:
        return str(state.props)
        
    def theorySetup(self, theoryClasses):
        rank = 0
        mt = 0
        for theoryGroup in theoryClasses:
            for tag in theoryGroup:
                if 'utility'==tag:
                    mt = Utility()
                elif 'law'==tag:
                    mt = TheLaw()
                else:
                    raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
                mt.rank = rank
                self.Theories.append(mt)
            rank+=1
    def optionsString():
        s = ""
        return s + "Theory options are `utility`, `law`."