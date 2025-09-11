from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP
from EnvironmentBuilder.LostInsulin.InsulinTheories import *
import random


class LostInsulin(MDP):

    def __init__(self, initialProps=None, horizon=20, budget=18.5, theoryClasses=[['time']], **kwargs):
        super().__init__()
        if initialProps==None:
            initialProps=LostInsulin.defaultProps
        if not horizon==None:
            initialProps['horizon'] = horizon
        self.horizon=horizon
        
        self.stateFactory(initialProps) # Create at least one initial state
        self.rules = [LostInsulin.ToGive, LostInsulin.ToSteal, LostInsulin.LeaveOrWait, LostInsulin.DeathChance] 
        self.Theories = []
        self.theorySetup(theoryClasses)
        self.CostTheory = Time(self.horizon)
        self.budget = budget
        self.isNonMoral = False

    defaultProps = {
            'time':0,
            'Hal_alive': True,
            'Carla_alive': True,
            'Hal_has_insulin': False,
            'Carla_has_insulin': True,
            'Carla_compensated': False,
            'found_insulin': False,
            'Hal_arrested': False,
            'Hal_at': 'Hal_house',# Hal can be at his house, Carla's house: 'Hal_house', 'Carla_house'
            'Hal_neutral': False
        }

    def isGoal(self, state):
        # Non-moral goal
        return state.props['Hal_has_insulin']


    def getActions(self, state):
        if (state.props['time']>=self.horizon):
            return []
        if (state.props['Hal_alive']==False or state.props['Hal_neutral'] or state.props['Hal_arrested']):
            return ['wait']
        
        if (state.props['Hal_at']=='Hal_house'):
            return ['go_to_Carla', 'wait']
        
        if (state.props['Hal_at']=='Carla_house' and state.props['found_insulin']==False):
            return ['give_low', 'give_high', 'leave']
        
        if (state.props['Hal_at']=='Carla_house' and state.props['found_insulin']==True and state.props['Hal_has_insulin']==False and state.props['Carla_has_insulin']==True):
            return ['steal', 'leave']
        if (state.props['Hal_at']=='Carla_house' and state.props['found_insulin']==True and state.props['Carla_has_insulin']==False):
            return ['leave', 'wait']
        
        return ['wait']

    #def isGoal(self, state):
    #    return False

    # Second choices: compensate low, high, nothing, or leave.
    def ToGive(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        if (not (props['Hal_at']=='Carla_house' and props['found_insulin']==False)):
            return [(deepcopy(props), prob)]
        if (props['Hal_arrested']):
            return [(deepcopy(props), prob)]
        if (not props['Hal_alive']):
            return [(deepcopy(props), prob)]
            
        outcomes = []
        
        if (action=='give_low'):
            p_ = deepcopy(props)
            p_['Carla_compensated']=False
            p_['found_insulin']=True
            outcomes.append((p_, prob*0.9))
            p_ = deepcopy(props)
            p_['Carla_compensated']=True
            p_['found_insulin']=True
            outcomes.append((p_, prob*0.1))
            
        elif (action=='give_high'):
            p_ = deepcopy(props)
            p_['Carla_compensated']=False
            p_['found_insulin']=True
            outcomes.append((p_, prob*0.3))
            p_ = deepcopy(props)
            p_['Carla_compensated']=True
            p_['found_insulin']=True
            outcomes.append((p_, prob*0.7))
            
        elif (action=="leave"):
            p_ = deepcopy(props)
            p_['Hal_at']='Hal_house'
            p_['Hal_neutral'] = True
            outcomes.append((p_, prob))
        
        return outcomes if len(outcomes)>0 else [(deepcopy(props), prob)]
    

    # Initial choices: go to Carla's or wait.
    def LeaveOrWait(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        if (props['Hal_arrested']):
            return [(deepcopy(props), prob)]
        if (props['Hal_at']!='Hal_house'):
            return [(deepcopy(props), prob)]
        
        outcomes = []
        if (action=='go_to_Carla'):
            p_ = deepcopy(props)
            p_['Hal_at'] = 'Carla_house'
            outcomes.append((p_, prob*0.8))
            
            p_ = deepcopy(props)
            p_['Hal_arrested'] = True
            outcomes.append((p_, prob*0.2))
        elif (action=='wait'):
            p_ = deepcopy(props)
            p_['Hal_neutral'] = True
            outcomes.append((p_, prob))

        
        return outcomes if len(outcomes)>0 else [(deepcopy(props), prob)]


    # Second choices: compensate low, high, nothing, or leave.
    def ToSteal(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        if (props['Hal_arrested']):
            return [(deepcopy(props), prob)]
        if (not (props['Hal_at']=='Carla_house' and props['found_insulin']==True)):
            return [(deepcopy(props), prob)]
        outcomes = []
        
        if (action=='steal'):
            p_ = deepcopy(props)
            p_['Hal_at']=='Hal_house'
            p_['Hal_has_insulin']=True
            p_['Carla_has_insulin']=False
            p_['Hal_neutral'] = True
            outcomes.append((p_, prob))
        elif (action=='leave'):
            p_ = deepcopy(props)
            p_['Hal_at']=='Hal_house'
            p_['Hal_neutral'] = True
            outcomes.append((p_, prob))
        
        return outcomes if len(outcomes)>0 else [(deepcopy(props), prob)]

    def DeathChance(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        outcomes = []
        
        if (props['Hal_alive']!=False and props['Hal_has_insulin']==False):
            p_ = deepcopy(props)    
            p_['Hal_alive']=True
            p_['time']+=1
            outcomes.append((p_, prob*0.4))
            p_ = deepcopy(props)    
            p_['Hal_alive']=False
            p_['time']+=1
            outcomes.append((p_, prob*0.6))
            
        if (props['Carla_alive']!=False and props['Carla_has_insulin']==False):
            p_ = deepcopy(props)
            p_['time']+=1
            p_['Carla_alive']=True
            outcomes.append((p_, prob*0.9))
            p_ = deepcopy(props)    
            p_['time']+=1
            p_['Carla_alive']=False
            outcomes.append((p_, prob*0.1))
            
        if (len(outcomes)>0):
            return outcomes
        else:
            p_ = deepcopy(props)    
            p_['time']+=1
            outcomes.append((p_, prob))
            return outcomes


    def terminateRule(self, props, prob, action):
        props_ = deepcopy(props)

        return [(props_,prob*1)]




    # Setup stuff.
    def stateString(self, state) -> str:
        return str(state.props)
        
    def theorySetup(self, theoryClasses):
        rank = 0
        mt = 0
        for theoryGroup in theoryClasses:
            for tag in theoryGroup:
                if 'time'==tag:
                    mt = Time(self.horizon)
                elif 'overall'==tag:
                    mt = OverallUtility()
                elif 'LifeAndDeath'==tag:
                    mt = LifeAndDeath()
                elif 'ToSteal'==tag:
                    mt = ToSteal()
                elif 'StealWithComp'==tag:
                    mt = StealWithComp()
                elif 'HalLife'==tag:
                    mt = HalLife()
                elif 'CarlaLife'==tag:
                    mt = CarlaLife()
                elif 'Cost'==tag:
                    self.isNonMoral=True
                    mt = Time(self.horizon)
                else:
                    raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
                mt.rank = rank
                self.Theories.append(mt)
            rank+=1
    def optionsString():
        s = "Initial property options are \n{`xy`: [int], `max_xy`: [int], `walls`:`max_xy`: [[int,int]], `playgrounds`:`max_xy`: [[int,int]], `goals`:`max_xy`: [[int,int]]} \n"
        return s + "Theory options are `time`, `avoid_playgrounds`, `avoid_checkpoints`."