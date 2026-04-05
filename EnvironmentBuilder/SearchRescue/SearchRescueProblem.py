from copy import deepcopy
from EnvironmentBuilder.BaseMDP import *
import numpy as np
class Cost(Consideration):
    def __init__(self, deadline):
        super().__init__()
        self.type='Cost'
        self.tag='Cost'
        self.deadline = deadline
        self.rank=0
        self.default = 0

    def judge(self, successor: Successor):
        src_props = successor.sourceState.props
        tar_props = successor.targetState.props

        # Must be back at base at the deadline.
        if (tar_props["time"]==self.deadline and tar_props['tile_state'][tar_props["curr_tile"]] != "base"):
            return -999999999
        
        return -1
        
        
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


class Odds:
    Pr_Treat_Success = 0.5

    Pr_Find_Hunger = 0.5
    Pr_Find_Disease = 0.5

class SearchRescue(MDP):

    AdjEdge = {}
    Community = []


    defaultProps = {
        "time":0,
        "tile_state": [], # status of current tile
        "holding": 'None',
        "curr_tile": 0,
    }

    def GenerateGraph(nodes=5):
        MaxOutLinks = 3
        SearchRescue.AdjEdge = {}
        if nodes <= 0:
            return
        for i in range(nodes):
            SearchRescue.AdjEdge.setdefault(str(i), [])

        # Start with a directed cycle to guarantee strong connectivity
        cycle = list(range(nodes))
        np.random.shuffle(cycle)
        for i in range(nodes):
            src = cycle[i]
            dst = cycle[(i + 1) % nodes]
            SearchRescue.AdjEdge[str(src)].append(dst)

        # Add some extra random edges while keeping strong connectivity
        for i in range(nodes):
            existing = set(SearchRescue.AdjEdge[str(i)])
            potential = [j for j in range(nodes) if j != i and j not in existing]
            if not potential:
                continue
            # Randomly choose number of extra links (0..len(potential))
            extra_count = np.random.randint(0, min(MaxOutLinks - 1, len(potential) + 1))
            if extra_count > 0:
                extras = list(np.random.choice(potential, size=extra_count, replace=False))
                SearchRescue.AdjEdge[str(i)].extend(extras)

        SearchRescue.initialProps["tile_state"] = ["?" for i in range(len(SearchRescue.AdjEdge))]
        SearchRescue.initialProps["tile_state"][0] = "base"

    def BuildMyGraph():
        SearchRescue.AdjEdge = {
            0:[1,6],
            1:[0,2,7],
            2:[1,3],
            3:[2],
            4:[3,5,6],
            5:[4,6],
            6:[0,4],
            7:[1],
        }
        SearchRescue.Community = ["red","red","red","red","blue","blue","blue", ""]
        SearchRescue.initialProps["tile_state"] = ["?","?","?","?","?","?","?","base"]


    def __init__(self, Theories, Considerations, initialProps=None, Horizon=5, Budget=5, **kwargs):
        super().__init__()
        if initialProps==None:
            initialProps=SearchRescue.defaultProps

        if not Horizon==None:
            initialProps['horizon'] = Horizon
        self.stateFactory(initialProps) # Create at least one initial state

        self.rules = [SearchRescue.Move, 
                      SearchRescue.InteractBase, 
                      SearchRescue.InteractInjured, 
                      SearchRescue.InteractUnknown, 
                      SearchRescue.AdvanceTime
                      ] 

        self.budget = Budget
        self.MaxOutsideTime = 3
        self.horizon=Horizon

        self.Theories = []
        self.theorySetup(Theories, Considerations)
        
        


    def isGoal(self, state:State) -> bool:
        # returned/remained at original position.
        return state.props['tile_state'][state.props["curr_tile"]] == "base"

    
    def getActions(self, state:State) -> list:
        acts = []
        if (state.props['time'] >= self.horizon):
            return acts

            
        for next_tile in SearchRescue.AdjEdge[str(state.props["curr_tile"])]:
            acts.append(f"go_to:{next_tile}")
        acts.append("wait")
        currTile = state.props["tile_state"][state.props["curr_tile"]]
        holding =  state.props["holding"]
        if currTile == "base":
            if state.props['holding'] == "None":
                acts.append("get_medicine")
                acts.append("get_food")

        elif "hunger" in currTile and holding == "food":
            acts.append("feed")

        elif "disease" in currTile and holding == "medicine":
            acts.append("medicate")
        
        elif "disease" in currTile and holding != "medicine":
            acts.append("treat")

        elif currTile != "blank" and currTile != "helped":
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
        if (props["tile_state"][tileIdx] != "base"):
            return [(props, prob)]
        
        if action=="get_food":
            props["holding"] = "food"

        if action=="get_medicine":
            props["holding"] = "medicine"
            
        return [(props, prob)]
    

    def InteractHunger(self, props, prob, action:str):
        if action=="feed":
            props["tile_state"][props["curr_tile"]] = "None"
        
        return [(props, prob)]
    
    def InteractDisease(self, props, prob, action:str):
        if action=="medicate":
            props["tile_state"][props["curr_tile"]] = "None"
            return [(props, prob)]

        if action=="treat":
            o = []
            props["tile_state"][props["curr_tile"]] = "None"
            o.append((props, prob*Odds.Pr_Treat_Success))

            props["tile_state"][props["curr_tile"]] = "disease"
            o.append((props, prob*(1 - Odds.Pr_Treat_Success)))
        
    
    def InteractUnknown(self, props, prob, action:str):
        tileIdx = props["curr_tile"]
        if (action != "interact" or props["tile_state"][tileIdx] != "?"):
            return [(props, prob)]
        
        outcomes = []
        props["tile_state"][tileIdx] = "None"
        outcomes.append((props, prob * 0.4))

        com = SearchRescue.Community[tileIdx]
        props_ = deepcopy(props)
        props_["tile_state"][tileIdx] = f"{com}:injured"
        outcomes.append((props_, prob * 0.3))

        props_ = deepcopy(props)
        props_["tile_state"][tileIdx] = f"{com}:severe"
        outcomes.append((props_, prob * 0.3))

        return outcomes

    # time advances each transition
    def AdvanceTime(self, props, prob, action):
        props_ = deepcopy(props)
        props_["time"] = props_["time"] + 1
        
        #for i in range(len(props["tile_time"])):
            #props["tile_time"][i] += 1 Disabled for now

        return [(props_, prob)]

    
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
                mc = Cost(deadline=self.horizon)
            elif 'FindInfo'==tag:
                mc = FindInfo()
            elif 'NeverIgnore'==tag:
                mc = NeverIgnore()
            else:
                raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
            mc.componentOf = c["Component_of"]
            self.Considerations.append(mc)