from copy import deepcopy
from EnvironmentBuilder.BaseMDP import *


class Cost(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Cost'
        self.tag='Cost'
        self.rank=0
        self.default = 0

    def judge(self, successor: Successor):
        src_props = successor.sourceState.props
        tar_props = successor.targetState.props

        for i in range(len(src_props["tile_state"])):
            if src_props["tile_state"][i] ==  f"hunger{self.groupTag}":
                u -= 1 + (0.1 * src_props["tile_time"][i])
        
        
    def StateHeuristic(self, state:State):
        return 0

class FindInfo(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.tag='FindInfo'
        self.rank=0
        self.default = 0

    def judge(self, successor: Successor):
        src_props = successor.sourceState.props
        tar_props = successor.targetState.props
        u = 0 
        for i in range(len(src_props["tile_state"])):
            if tar_props["tile_state"][i] ==  "?":
                u -= 1
        return u
        
        
    def StateHeuristic(self, state:State):
        return 0


class NeverIgnore(Consideration):
    def __init__(self):
        super().__init__()
        self.type = ConsiderationTypes.ABSOLUTISM
        self.rank=0
        self.tag="NeverIgnore"
        self.default = 0

    def judge(self, successor: Successor):
        src_props = successor.sourceState.props
        tar_props = successor.targetState.props
        
        # If there is hunger and agent moves (not interacting)
        if ("hunger" in src_props['tile_state'][src_props['curr_tile']] and src_props['curr_tile'] != tar_props['curr_tile'] ):
            return True # violation
        
        # Same for injured
        if ("injured" in src_props['tile_state'][src_props['curr_tile']] and src_props['curr_tile'] != tar_props['curr_tile'] ):
            return True # violation

        return False

    def StateHeuristic(self, state:State):
        return False
    
class Wellbeing(Consideration):
    def __init__(self, group="all", tag='Utility'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0
        self.groupTag = f"{group}:" if group != "all" else ""

    def judge(self, successor: Successor):
        u = 0
        src_props = successor.sourceState.props
        tar_props = successor.targetState.props

        # lose for all hurt people on map
        for i in range(len(src_props["tile_state"])):
            if tar_props["tile_state"][i] ==  f"{self.groupTag}injured":
                u -= 1
            
            if tar_props["tile_state"][i] ==  f"{self.groupTag}severe":
                u -= 2

        # Benefit for helping people.
        # Can make this change with time? More severe with time        
        for i in range(len(src_props["tile_state"])):

            if src_props["tile_state"][i] == f"{self.groupTag}injured" and tar_props["tile_state"][i] == "helped":
                u += 1
            if src_props["tile_state"][i] == f"{self.groupTag}severe" and tar_props["tile_state"][i] == "helped":
                u += 2
        return u

    def StateHeuristic(self, state:State):
        return 0


class SearchRescue(MDP):
    events = [
        {
            "time":0,
            "tile":1,
            "type": "hunger:red",
            "probability":1,
            "context_prob": "0.5"
         },
         {
            "time":0,
            "tile":4,
            "type": "injured:blue",
            "probability":1,
            "context_prob": "0.5"
            
         }
    ]

    defaultProps = {
        "time":0,
        #"tile_state": ["base","hunger:red","blank","blank","injured:blue"], # status of current tile
        "tile_state": ["base","?","?","?","?"], # status of current tile
        "holding": 'None',
        "surgery_result": 'None',
        "tile_time": [-1, -1, -1, -1, -1], # time tile has had this status
        "curr_tile": 0,
        "battery": 4,
        "adjEdge": {
            "0": [1,3], #tile 0 can go to tile 1 or tile 3.   
            "1": [2],
            "2": [3,0],
            "3": [4],
            "4": [0],
        },
    }

    def __init__(self, Theories, Considerations, initialProps=None, horizon=5, **kwargs):
        super().__init__()
        if initialProps==None:
            initialProps=SearchRescue.defaultProps
            pass
        if not horizon==None:
            initialProps['horizon'] = horizon
        self.stateFactory(initialProps) # Create at least one initial state

        self.rules = [SearchRescue.Move, 
                      SearchRescue.InteractBase, 
                      SearchRescue.InteractInjured, 
                      SearchRescue.InteractUnknown, 
                      SearchRescue.AdvanceTime, 
                      #SearchRescue.AddEvents
                      ] 

        #self.CostTheory = Time()
        self.Theories = []
        self.theorySetup(Theories, Considerations)
        
        #self.budget = budget
        self.MaxOutsideTime = 3
        self.horizon=horizon


    def isGoal(self, state:State) -> bool:
        # returned/remained at original position.
        return state.props['tile_state'][state.props["curr_tile"]] == "base"

    
    def getActions(self, state:State) -> list:
        acts = []
        if (state.props['time'] >= self.horizon):
            return acts
        
        # Cannot move when battery is empty
        if state.props["battery"] == 0:
            return ["wait"]
            
        for next_tile in state.props["adjEdge"][str(state.props["curr_tile"])]:
            acts.append(f"go_to:{next_tile}")
        acts.append("wait")
        currTile = state.props["tile_state"][state.props["curr_tile"]]
        if currTile != "blank" and currTile != "helped":
            acts.append("interact")
            
        return acts

    def Move(self, props, prob, action:str):
        props_ = deepcopy(props)
        if (action.startswith("go_to")):
            props_["curr_tile"] = int(action.split(":")[1])
        return [(props_, prob)]

    
    # Charging results and surgery results
    def InteractBase(self, props, prob, action:str):
        tileIdx = props["curr_tile"]
        if (action != "interact" or props["tile_state"][tileIdx] != "base"):
            return [(props, prob)]
        
        outcomes = []
        props["battery"] += 2
        props["battery"] = 4 if props["battery"] > 4 else props["battery"]
        if (props["holding"] == "None"):
            return [(props, prob)]
        
        props_ = deepcopy(props)
        props_["surgery_result"] = f"success:{props_["holding"]}"
        outcomes.append((props_, prob * 0.5))

        props_ = deepcopy(props)
        props_["surgery_result"] = f"fail:{props_["holding"]}"
        outcomes.append((props_, prob * 0.5))
        return outcomes


    def InteractInjured(self, props, prob, action:str):
        tileIdx = props["curr_tile"]
        if (action != "interact"):
            return [(props, prob)]
        
        info = props["tile_state"][tileIdx].split(":")
        team, curr_tile_state = "", ""
        if len(info)>1:
            team = info[0]
            curr_tile_state = info[1]

        if (curr_tile_state != "injured" and curr_tile_state != "severe"):
            return [(props, prob)]


        outcomes = []
        
        curr_tile_timing = props["tile_time"][tileIdx]

        pr_helped = 0.5
        # lower chance to help if severe injury
        if (curr_tile_state=="severe"):
            pr_helped = 0.3
        
        
        props_ = deepcopy(props)
        props_["tile_state"][tileIdx] = "helped"
        props_["tile_time"][tileIdx] = -1
        outcomes.append((props_, prob * 0.5))

        props_ = deepcopy(props)
        props_["tile_state"][tileIdx] = "blank"
        props_["tile_time"][tileIdx] = -1
        outcomes.append((props_, prob * (1 - pr_helped)))

        return outcomes


    def InteractUnknown(self, props, prob, action:str):
        tileIdx = props["curr_tile"]
        if (action != "interact" or props["tile_state"][tileIdx] != "?"):
            return [(props, prob)]
        
        outcomes = []
        props["tile_state"][tileIdx] = "None"
        outcomes.append((props, prob * 0.4))

        props_ = deepcopy(props)
        props_["tile_state"][tileIdx] = "red:injured"
        outcomes.append((props_, prob * 0.3))

        props_ = deepcopy(props)
        props_["tile_state"][tileIdx] = "red:severe"
        outcomes.append((props_, prob * 0.3))

        return outcomes

    # time advances each transition
    def AdvanceTime(self, props, prob, action):
        props_ = deepcopy(props)
        props_["time"] = props_["time"] + 1
        
        #for i in range(len(props["tile_time"])):
            #props["tile_time"][i] += 1 Disabled for now

        return [(props_, prob)]

    def AddEvents(self, props, prob, action):
        outcomes = [(props, prob)]
        for e in SearchRescue.events:
            if e["time"]==props["time"]: # Because after AdvanceTime rule
                tile = e["tile"]
                # Skip if effect already applied.
                if props["tile_state"][tile] == e["type"]:
                    continue

                eventProb = 1
                if ("probability" in e.keys()):
                    eventProb = e["probability"]
                for i in range(len(outcomes)):
                    if (eventProb < 1):
                        # Make a copy and apply effect
                        props_, prob_ = deepcopy(outcomes[i])
                        props_["tile_time"][tile] = 0
                        props_["tile_state"][tile] = e["type"]
                        prob_*= eventProb
                        outcomes.append((props_, prob_))
                        # Add reverse probability to unaffected original
                        outcomes[i][1] *= (1 - eventProb)
                    else:
                        # apply effect to original
                        outcomes[i][0]["tile_time"][tile] = 0
                        outcomes[i][0]["tile_state"][tile] = e["type"]
                        outcomes[i][1] *= eventProb
        return outcomes


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
            if 'red:wellbeing'==tag:
                mc = Wellbeing(group='red', tag=tag)
            elif 'blue:wellbeing'==tag:
                mc = Wellbeing(group='blue', tag=tag)
            elif 'wellbeing'==tag:
                mc = Wellbeing(tag=tag)
            elif 'Cost'==tag:
                mc = Cost()
            elif 'FindInfo'==tag:
                mc = FindInfo()
            elif 'NeverIgnore'==tag:
                mc = NeverIgnore()
            else:
                raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
            mc.componentOf = c["Component_of"]
            self.Considerations.append(mc)