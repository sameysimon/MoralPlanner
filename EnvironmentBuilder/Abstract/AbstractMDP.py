from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP, Theory, Consideration, State
from EnvironmentBuilder.Abstract.AbstractTheories import *
import numpy as np
import random


class AbstractMDP(MDP):

    def __init__(self, Theories, Considerations, initialProps=None, Horizon=20, Budget=6, GoalP=0, BranchF=2, ActionF=2, **kwargs):
        super().__init__()
        random.seed(kwargs['Seed'])
        np.random.seed(kwargs['Seed'])
        if initialProps==None:
            initialProps=AbstractMDP.defaultProps
        if not Horizon==None:
            initialProps['horizon'] = Horizon
        self.stateFactory(initialProps) # Create at least one initial state
        self.rules = [AbstractMDP.Next] 
        #self.CostTheory = Time()

        self.theorySetup(Theories, Considerations)
        
        self.budget = Budget
        self.goalOdds = GoalP
        self.isNonMoral = GoalP>0

        self.horizon=Horizon
        self.branchFactor=BranchF
        self.actionFactor=ActionF

        self.myActions = []
        for i in range(ActionF):
            self.myActions.append('a_' + str(i))
    defaultProps = {
            'time':0,
            'utility0':0,
            'utility1':0,
            'utility2':0,
            'utility3':0,
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
            props_['utility0'] = random.uniform(-10, 10)
            props_['utility1'] = random.uniform(-10, 10)
            props_['law'] = random.random() < 0.5
            if (props_['time']>=self.horizon/2):
                props_['isGoal']=random.random()>self.goalOdds
            outs.append((props_, prob * probDist[i]))
        return outs



    # Setup stuff.
    def stateString(self, state) -> str:
        return str(state.props)
        
    def theorySetup(self, theories:list[Theory], considerations):
        for t in theories:
            mt = Theory()
            mt.name = t["Name"]
            mt.rank = t["Rank"]
            mt.type = t["Type"]
            self.Theories.append(mt)

        rank = 0
        mc = 0
        conId = 0
        for c in considerations:
            if 'utility' in c["Type"]:
                mc = Utility(conId)
            elif 'law' in c["Type"]:
                mc = TheLaw(conId)
            else:
                raise Exception('Moral consideration with type ' + c["Type"] + ' at rank ' + str(rank) + ' invalid.')
            
            mc.componentOf=c["Component_of"]
            self.Considerations.append(mc)
            conId+=1
            rank+=1

    def optionsString():
        s = ""
        return s + "Theory options are `utility`, `law`."