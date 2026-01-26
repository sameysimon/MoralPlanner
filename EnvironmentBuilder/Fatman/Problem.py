from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP
import random

'''
Fatman trolley problem.
Trolley's position controlled by trolley_x.
    Starts at 0. Hits at 1. Terminates at 2
Swtich determines where the trolley is heading. Either GROUP or PERSON

'''

class FatmanProblem(MDP):
    prob_fatDeath= 0.8
    prob_GrpDeath= 0.8
    prob_PerDeath= 0.8

    def __init__(self,**kwargs) -> None:
        kwargs.setdefault('horizon', 4)
        kwargs.setdefault('budget', 8)
        kwargs.setdefault('theoryClasses', [['time']])
        kwargs.setdefault('initialProps', None)
        kwargs.setdefault('switch_enabled', True)
        kwargs.setdefault('fatman_enabled', True)
        self.isFatman = kwargs['fatman_enabled']
        self.isSwitch = kwargs['switch_enabled']

        super().__init__()
        if kwargs['initialProps']==None:
            initialProps=FatmanProblem.defaultProps
        self.horizon=kwargs['horizon']
        self.budget=kwargs['budget']

        self.stateFactory(initialProps) # Create at least one initial state
        self.rules = [FatmanProblem.ActionHandler, FatmanProblem.CollisionHandler, FatmanProblem.TimeHandler] 
        # self.CostTheory = Time()
        self.Theories = []
        self.theorySetup(kwargs['theoryClasses'])
    
    defaultProps = {
            "trolley_x": 0,
            "time": 0,
            "fatman_alive": True,
            "fatman_on_bridge": True,
            "group_alive": True,
            "person_alive": True,
            "switch": "GROUP"
        }

    def getActions(self, state):
        if (state.props['trolley_x']==2):
            return []
        a = []
        if (state.props['fatman_on_bridge'] and self.isFatman):
            a.append("PUSH")

        if (self.isSwitch):
            a.append("SWITCH")

        a.append("WAIT")
        return a        

    def isGoal(self, state):
        return False

    # standard move action
    def ActionHandler(self, props, prob, action):
        props_ = deepcopy(props)
        if (action=="PUSH" and props["fatman_on_bridge"]):
            props_["fatman_on_bridge"]=False

        if (action=="SWITCH" and props["switch"]=="GROUP"):
            props_["switch"]="PERSON"

        if (action=="SWITCH" and props["switch"]=="PERSON"):
            props_["switch"]="GROUP"


        return [(props_, prob*1)]
    
    def addGroupHitOutcomes(self, props, prob):
        outs = []
        # TRAIN HITS GROUP
        props_ = deepcopy(props)
        props_["group_alive"]= False
        outs.append((props_, prob * self.prob_GrpDeath))
        # TRAIN MISSES GROUP
        props_ = deepcopy(props)
        outs.append((props_, prob * (1 - self.prob_GrpDeath)))
        return outs
   
    def addPersonHitOutcomes(self, props, prob):
        outs = []
        # TRAIN HITS PERSON
        props_ = deepcopy(props)
        props_["person_alive"]= False
        outs.append((props_, prob * self.prob_PerDeath))
        # TRAIN MISSES PERSON
        props_ = deepcopy(props)
        outs.append((props_, prob * (1 - self.prob_PerDeath)))
        return outs


    def CollisionHandler(self, props, prob, action):
        if (props['trolley_x']!=1):
            return [(props, prob*1)]
        
        outcomes = []
        # FATMAN PUSHED
        if (not props["fatman_on_bridge"]):
            # FATMAN LIVES, TRAIN CONTINUES.
            if (props["switch"]=="GROUP"):
                outcomes.extend(self.addGroupHitOutcomes(props, prob * (1 - self.prob_fatDeath)))
            if (props["switch"]=="PERSON"):
                outcomes.extend(self.addPersonHitOutcomes(props, prob * (1 - self.prob_fatDeath)))

            # FATMAN DIES, TRAIN CONTINUES
            props_ = deepcopy(props)
            props_["fatman_alive"]=False
            if (props["switch"]=="GROUP"):
                outcomes.extend(self.addGroupHitOutcomes(props_, prob * (self.prob_fatDeath)))
            if (props["switch"]=="PERSON"):
                outcomes.extend(self.addPersonHitOutcomes(props_, prob * (self.prob_fatDeath)))

        # FATMAN ON BRIDGE
        else:
            if (props["switch"]=="GROUP"):
                outcomes.extend(self.addGroupHitOutcomes(props, prob))

            if (props["switch"]=="PERSON"):
                outcomes.extend(self.addPersonHitOutcomes(props, prob))

                    
        return outcomes

    def TimeHandler(self, props, prob, action): 
        props_ = deepcopy(props)   
        props_['time']+=1
        return [(props_, prob)]
        




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