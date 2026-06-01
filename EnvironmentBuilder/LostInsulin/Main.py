from copy import deepcopy
from EnvironmentBuilder.BaseMDP import MDP, Theory
from EnvironmentBuilder.LostInsulin.InsulinTheories import *
import random


class Odds():
    CARLA_COMPLIES=0.2
    CARLA_BUYS_LOW=0.4
    CARLA_BUYS_HIGH=0.5
    CARLA_INTIMIDATED=0.5

    SNEAK_IN_ARREST=0.2
    BREAK_IN_ARREST=0.4

    ARREST_GIVES_INSULIN = 0.2
    
    COMPENSATE_LOW=0.4
    COMPENSATE_HIGH=0.5

    HAL_DIES=0.4
    CARLA_DIES=0.2

    HAL_FINDS=0.5

    CARLA_LOSES_FIGHT=0.7
    CARLA_LOSES_FIGHT_AFTER_INTIM=0.5
    HAL_FINDS_ENTRANCE=0.8
    
def SafeRemove(l:list, item):
    if item in l:
        l.remove(item)

class LostInsulin(MDP):

    def __init__(self, Theories, Considerations, InitialProps=None, Horizon=10, Budget=8, **kwargs):
        super().__init__()
        if InitialProps==None:
            InitialProps=LostInsulin.defaultProps
        if not Horizon==None:
            InitialProps['horizon'] = Horizon
        self.horizon=Horizon
        
        self.stateFactory(InitialProps) # Create at least one initial state
        self.rules = [LostInsulin.DeathChance, LostInsulin.ToCompensate, LostInsulin.LeaveOrWait, LostInsulin.Time] 
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
            'Carla_reply': 'na', # Can be 'na', 'gave' (sets Hal_has_insulin to True), 'refused_ask', 'refused_intimidate', 'refused_attack'
            'Carla_sold': 'na', # Can be 'na' if he hasn't tried | 'sold' | 'refused_low' | 'refused_high' 
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
            acts = ['ask_Carla', 'search_outside', 'break_in', 'wait', 'intimidate', 'attack_Carla', 'buy_low', 'buy_high', 'sneak_inside']
            
            if (state.props['Entry_status']!='easy_entry'):
                SafeRemove(acts, 'sneak_inside')
            
            if (state.props['Carla_sold']=='refused_low'):
                SafeRemove(acts, 'buy_low')
                SafeRemove(acts, 'ask_Carla')

            if (state.props['Carla_sold']=='refused_high'):
                SafeRemove(acts, 'buy_low')
                SafeRemove(acts, 'buy_high')
                SafeRemove(acts, 'ask_Carla')

            if (state.props['Entry_status']=='No_easy_entry' or state.props['Entry_status']=='easy_entry'):
                SafeRemove(acts, 'search_outside')

            if (state.props['Carla_reply']=='refused_ask'):
                SafeRemove(acts, 'ask_Carla')

            if (state.props['Carla_reply']=='refused_intimidate'):
                SafeRemove(acts, 'intimidate')
                SafeRemove(acts, 'ask_Carla')
                SafeRemove(acts, 'buy_low')
                SafeRemove(acts, 'buy_high')

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
        if (not props['Hal_alive']):
            return [(deepcopy(props), prob)]
        
        outcomes = []
        if (action=='break_in'):
            success_prob = Odds.BREAK_IN_ARREST

            p_ = deepcopy(props)
            p_['Hal_at'] = 'Carla_house'
            outcomes.append((p_, prob*success_prob))
            
            p_ = deepcopy(props)
            p_['Hal_arrested'] = True
            outcomes.append((p_, prob*(1 - success_prob) * (1-Odds.ARREST_GIVES_INSULIN)))

            p_ = deepcopy(props)
            p_['Hal_arrested'] = True
            p_['Hal_has_insulin'] = True
            outcomes.append((p_, prob*(1 - success_prob) * Odds.ARREST_GIVES_INSULIN))
        elif (action=='sneak_inside'):
            p_ = deepcopy(props)
            p_['Hal_at'] = 'Carla_house'
            outcomes.append((p_, prob*(1-Odds.SNEAK_IN_ARREST)))
            
            p_ = deepcopy(props)
            p_['Hal_arrested'] = True
            outcomes.append((p_, prob*Odds.SNEAK_IN_ARREST * (1-Odds.ARREST_GIVES_INSULIN)))

            p_ = deepcopy(props)
            p_['Hal_arrested'] = True
            p_['Hal_has_insulin'] = True
            outcomes.append((p_, prob * Odds.SNEAK_IN_ARREST * Odds.ARREST_GIVES_INSULIN))


        elif (action=='ask_Carla'):
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'gave'
            p_['Hal_has_insulin'] = True
            p_['Carla_has_insulin']=False
            outcomes.append((p_, prob * Odds.CARLA_COMPLIES))
            
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'refused_ask'
            outcomes.append((p_, prob * (1 - Odds.CARLA_COMPLIES)))
        elif (action=='buy_low'):
            p_ = deepcopy(props)
            p_['Carla_sold'] = 'sold'
            p_['Hal_has_insulin'] = True
            p_['Carla_has_insulin']=False
            # Reset these variables since they won't be relevant
            p_['Carla_reply'] = 'na'
            p_['Entry_status'] = 'na'
            outcomes.append((p_, prob * Odds.CARLA_BUYS_LOW))

            p_ = deepcopy(props)
            p_['Carla_sold'] = 'refused_low'
            outcomes.append((p_, prob * (1 - Odds.CARLA_BUYS_LOW)))
        elif (action=='buy_high'):
            p_ = deepcopy(props)
            p_['Carla_sold'] = 'sold'
            p_['Hal_has_insulin'] = True
            p_['Carla_has_insulin']=False
            # Reset these variables since they won't be relevant
            p_['Carla_reply'] = 'na'
            p_['Entry_status'] = 'na'

            outcomes.append((p_, prob * Odds.CARLA_BUYS_HIGH))

            p_ = deepcopy(props)
            p_['Carla_sold'] = 'refused_high'
            outcomes.append((p_, prob * (1 - Odds.CARLA_BUYS_HIGH)))

        elif (action=='intimidate'):
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'gave'
            p_['Hal_has_insulin'] = True
            p_['Carla_has_insulin']=False
            # Reset these variables since they won't be relevant
            p_['Entry_status'] = 'na'
            outcomes.append((p_, prob * Odds.CARLA_INTIMIDATED))
            
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'refused_intimidate'
            outcomes.append((p_, prob * (1 - Odds.CARLA_INTIMIDATED)))
        elif (action=='attack_Carla'):
            p_ = deepcopy(props)
            odds = Odds.CARLA_LOSES_FIGHT_AFTER_INTIM if p_['Carla_reply']=='refused_intimidate' else Odds.CARLA_LOSES_FIGHT
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'gave'
            p_['Hal_has_insulin'] = True
            p_['Carla_has_insulin']=False
            
            outcomes.append((p_, prob * odds))
            
            p_ = deepcopy(props)
            p_['Carla_reply'] = 'refused_attack'
            outcomes.append((p_, prob * (1 - odds)))
        elif (action=='search_outside'):
            p_ = deepcopy(props)
            p_['Entry_status'] = 'easy_entry'
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
        props['Entry_status']='inside'
        props['Carla_reply']='na'


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
            die_chance = 1 - ((((self.horizon - props['time']) ) / (self.horizon + 1)) * (1 - Odds.HAL_DIES))
            

            p_ = deepcopy(props)    
            p_['Hal_alive']=True
            outcomes.append((p_, prob*(1 - die_chance)))
            p_ = deepcopy(props)    
            p_['Hal_alive']=False
            outcomes.append((p_, prob*die_chance))
            
        if (props['Carla_alive'] and props['Carla_has_insulin']==False):
            p_ = deepcopy(props)
            p_['Carla_alive']=True
            outcomes.append((p_, prob*(1 - Odds.CARLA_DIES)))

            p_ = deepcopy(props)    
            p_['Carla_alive']=False
            outcomes.append((p_, prob*Odds.CARLA_DIES))
            
        if (len(outcomes)>0):
            return outcomes
        else:
            outcomes.append((props, prob))
            return outcomes

    def Time(self, props, prob, action):
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
            if 'time'==tag:
                mc = Time(self.horizon)
            elif 'Overall'==tag:
                mc = OverallUtility()
            elif 'ToSteal'==tag:
                mc = ToSteal()
            elif 'StealWithComp'==tag:
                mc = StealWithComp()
            elif 'HalLife'==tag:
                mc = HalLife()
            elif 'CarlaLife'==tag:
                mc = CarlaLife()
            elif 'Hal'==tag:
                mc = Hal()
            elif 'Carla'==tag:
                mc = Carla()
            elif 'HalSmall'==tag:
                mc = HalSmall()
            elif 'CarlaSmall'==tag:
                mc = CarlaSmall()
            elif 'Cost'==tag:
                self.isNonMoral=True
                mc = Time(self.horizon, self.budget)
                self.CostTheory = mc
            elif 'Necessity'==tag:
                mc = OrdinalNecessity(0, horizon=self.horizon)
            elif 'OrdinalLaw'==tag:
                mc = OrdinalLaw(0)
            else:
                raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
            mc.componentOf=c["Component_of"]
            self.Considerations.append(mc)

    def optionsString():
        s = "Initial property options are \n{`xy`: [int], `max_xy`: [int], `walls`:`max_xy`: [[int,int]], `playgrounds`:`max_xy`: [[int,int]], `goals`:`max_xy`: [[int,int]]} \n"
        return s + "Theory options are `time`, `avoid_playgrounds`, `avoid_checkpoints`."