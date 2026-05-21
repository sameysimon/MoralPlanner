from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP, Theory
from EnvironmentBuilder.LostInsulin.InsulinTheories import *
import random


class Odds():
    CARLA_COMPLIES=0.8
    CARLA_INTIMIDATED=0.5
    REFUSED_BREAK_IN=0.5
    FIRST_BREAK_IN=0.8
    COMPENSATE_LOW=0.1
    COMPENSATE_HIGH=0.1
    HAL_DIES=0.4
    CARLA_DIES=0.1
    HAL_FINDS=0.5
    CARLA_LOSES_FIGHT=0.5
    HAL_FINDS_ENTRANCE=0.5
    


class LostInsulin(MDP):

    def __init__(self, Theories, Considerations, InitialProps=None, Horizon=10, Budget=8, **kwargs):
        super().__init__()
        if InitialProps==None:
            InitialProps=LostInsulin.defaultProps
        if not Horizon==None:
            InitialProps['horizon'] = Horizon
        self.horizon=Horizon
        
        self.stateFactory(InitialProps) # Create at least one initial state
        self.rules = [LostInsulin.ToCompensate, LostInsulin.LeaveOrWait, LostInsulin.DeathChance] 
        self.Theories = []
        self.budget = Budget
        self.isNonMoral = False
        self.theorySetup(Theories, Considerations)

    defaultProps = {
            'time':0,
            'Hal_alive': True,
            'Carla_alive': True,
            'Hal_has_insulin': False,
            'Carla_has_insulin': True,
            'Carla_compensated': False,
            'Hal_arrested': False,
            'Hal_at': 'Hal_house',# Can be 'Hal_house', 'Carla_house'
            'Carla_reply': 'na', # Can be 'na', 'gave' (sets Hal_has_insulin to True), 'refused_ask', 'refused_intimidate', 'refuse_attack'.
            'Entry_status': 'none' # Can be 'No_easy_entry' | 'broken_in' | 'done_violent_entry'
        }

    def isGoal(self, state):
        # Non-moral goal
        return state.props['Hal_has_insulin']


    def getActions(self, state):
        if (state.props['time']>=self.horizon):
            return []
        if (state.props['Hal_alive']==False):
            return ['dead']
        if (state.props['Hal_arrested']):
            return ['arrested']
        if (state.props['Hal_has_insulin']):
            return ['safe']
        if (state.props['Carla_reply']=='refused_attack'):
            return ['hurt']
        
        if (state.props['Hal_at']=='Hal_house'):
            acts = ['ask_Carla', 'break_in', 'search_outside', 'wait', 'intimidate', 'attack_Carla']
            if (state.props['Entry_status']=='No_easy_entry'):
                acts.remove('search_outside')

            if (state.props['Carla_reply']=='refused_ask'):
                acts.remove('ask_Carla')

            if (state.props['Carla_reply']=='refused_intimidate'):
                acts.remove('intimidate')
                acts.remove('ask_Carla')

            if (state.props['Carla_reply']=='refused_attack'):
                acts.remove('intimidate')
                acts.remove('attack_Carla')
                acts.remove('ask_Carla')
            return acts
        
        if (state.props['Hal_at']=='Carla_house'):
            if (state.props['Hal_has_insulin']==False and state.props['Carla_has_insulin']==True):
                return ['steal', 'give_low', 'give_high', 'leave']
            if (state.props['Carla_has_insulin']==False):
                return ['leave', 'wait']

        return ['wait']


        

    # Initial choices: go to Carla's or wait.
    def LeaveOrWait(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        if (props['Hal_arrested']):
            return [(deepcopy(props), prob)]
        if (props['Hal_at']!='Hal_house'):
            return [(deepcopy(props), prob)]
        
        outcomes = []
        if (action=='break_in'):
            success_prob = Odds.FIRST_BREAK_IN if props['Carla_reply']=='na' else Odds.REFUSED_BREAK_IN

            p_ = deepcopy(props)
            p_['Hal_at'] = 'Carla_house'
            p_['Entry_status'] = 'broken_in'
            outcomes.append((p_, prob*success_prob))
            
            p_ = deepcopy(props)
            p_['Hal_arrested'] = True
            outcomes.append((p_, prob*(1 - success_prob)))
        elif (action=='ask_Carla'):
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'gave'
            p_['Hal_has_insulin'] = True
            outcomes.append((p_, prob * Odds.CARLA_COMPLIES))
            
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'refused_ask'
            outcomes.append((p_, prob * (1 - Odds.CARLA_COMPLIES)))
        elif (action=='intimidate'):
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'gave'
            p_['Hal_has_insulin'] = True
            outcomes.append((p_, prob * Odds.CARLA_INTIMIDATED))
            
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'refused_intimidate'
            outcomes.append((p_, prob * (1 - Odds.CARLA_INTIMIDATED)))
        elif (action=='attack_Carla'):
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'gave'
            p_['Hal_has_insulin'] = True
            outcomes.append((p_, prob * Odds.CARLA_LOSES_FIGHT))
            
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'refused_attack'
            outcomes.append((p_, prob * (1 - Odds.CARLA_LOSES_FIGHT)))
        elif (action=='search_outside'):
            p_ = deepcopy(props)
            p_['Hal_at'] = 'Carla_house'
            outcomes.append((p_, prob*Odds.HAL_FINDS_ENTRANCE))
            
            p_ = deepcopy(props)
            p_['Entry_status'] = 'No_easy_entry'
            outcomes.append((p_, prob*(1 - Odds.HAL_FINDS_ENTRANCE)))
        
        return outcomes if len(outcomes)>0 else [(deepcopy(props), prob)]
    

    # Second choices: compensate low, high, nothing, or leave.
    def ToCompensate(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        if (not (props['Hal_at']=='Carla_house')):
            return [(deepcopy(props), prob)]
        if (props['Hal_arrested']):
            return [(deepcopy(props), prob)]
        if (not props['Hal_alive']):
            return [(deepcopy(props), prob)]
            
        outcomes = []
        
        if (action=='give_low'):
            p_ = deepcopy(props)
            p_['Carla_compensated']=False
            p_['Hal_has_insulin']=True
            p_['Carla_has_insulin']=False
            outcomes.append((p_, prob*(1 - Odds.COMPENSATE_LOW)))
            p_ = deepcopy(props)
            p_['Carla_compensated']=True
            p_['Hal_has_insulin']=True
            p_['Carla_has_insulin']=False
            outcomes.append((p_, prob*Odds.COMPENSATE_LOW))
            
        elif (action=='give_high'):
            p_ = deepcopy(props)
            p_['Carla_compensated']=False
            p_['Hal_has_insulin']=True
            p_['Carla_has_insulin']=False
            outcomes.append((p_, prob*(1 - Odds.COMPENSATE_HIGH)))
            p_ = deepcopy(props)
            p_['Carla_compensated']=True
            p_['Hal_has_insulin']=True
            p_['Carla_has_insulin']=False
            outcomes.append((p_, prob*Odds.COMPENSATE_HIGH))
        elif (action=='steal'):
            p_ = deepcopy(props)
            p_['Carla_compensated']=False
            p_['Hal_has_insulin']=True
            p_['Carla_has_insulin']=False
            outcomes.append((p_, prob))
        elif (action=="leave"):
            p_ = deepcopy(props)
            p_['Hal_at']='Hal_house'
            outcomes.append((p_, prob))
        
        return outcomes if len(outcomes)>0 else [(deepcopy(props), prob)]
    

    def DeathChance(self, props, prob, action):
        if (props['time']>=self.horizon):
            return []
        outcomes = []
        
        if (props['Hal_alive'] and props['Hal_has_insulin']==False):
            p_ = deepcopy(props)    
            p_['Hal_alive']=True
            p_['time']+=1
            outcomes.append((p_, prob*(1 - Odds.HAL_DIES)))
            p_ = deepcopy(props)    
            p_['Hal_alive']=False
            p_['time']+=1
            outcomes.append((p_, prob*Odds.HAL_DIES))
            
        if (props['Carla_alive'] and props['Carla_has_insulin']==False):
            p_ = deepcopy(props)
            p_['time']+=1
            p_['Carla_alive']=True
            outcomes.append((p_, prob*(1 - Odds.CARLA_DIES)))

            p_ = deepcopy(props)    
            p_['time']+=1
            p_['Carla_alive']=False
            outcomes.append((p_, prob*Odds.CARLA_DIES))
            
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
            if 'time'==tag:
                mc = Time(self.horizon)
            elif 'overall'==tag:
                mc = OverallUtility()
            elif 'LifeAndDeath'==tag:
                mc = LifeAndDeath()
            elif 'ToSteal'==tag:
                mc = ToSteal()
            elif 'StealWithComp'==tag:
                mc = StealWithComp()
            elif 'HalLife'==tag:
                mc = HalLife()
            elif 'CarlaLife'==tag:
                mc = CarlaLife()
            elif 'Cost'==tag:
                self.isNonMoral=True
                mc = Time(self.horizon, self.budget)
                self.CostTheory = mc
            elif 'Necessity'==tag:
                mc = OrdinalNecessity(0)
            elif 'OrdinalLaw'==tag:
                mc = OrdinalLaw(0)
            else:
                raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
            mc.componentOf=c["Component_of"]
            self.Considerations.append(mc)

    def optionsString():
        s = "Initial property options are \n{`xy`: [int], `max_xy`: [int], `walls`:`max_xy`: [[int,int]], `playgrounds`:`max_xy`: [[int,int]], `goals`:`max_xy`: [[int,int]]} \n"
        return s + "Theory options are `time`, `avoid_playgrounds`, `avoid_checkpoints`."