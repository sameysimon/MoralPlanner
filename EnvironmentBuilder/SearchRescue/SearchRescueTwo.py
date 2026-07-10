from copy import deepcopy
from EnvironmentBuilder.BaseMDP import *
import numpy as np
import random
import networkx as nx

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
        
        curr_tile_state = src_props['tile_state'][src_props['curr_tile']]
        agent_moves = src_props['curr_tile'] != tar_props['curr_tile']

        # If someone is hungry, and agent had food, must give food.
        if ((curr_tile_state=="hunger" and agent_moves) and src_props["holding"]=="food"):
            return True

        # If someone is severe and agent moves away, violation.
        if (curr_tile_state=="severe" and agent_moves):
            return True
        
        # If injured and agent moves away, violation.
        if (curr_tile_state=="injured" and agent_moves):
            return True 
        
        return False

    def StateHeuristic(self, state:State):
        return False

class NegativeWellbeing(Consideration):
    def __init__(self, group="all", group_idx=0, tag='Negative_Well-being'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0
        self.group = group
        self.group_idx = group_idx

    def judge(self, successor: Successor):
        if self.group=="all":
            return sum(successor.targetState.props["Comm_wellbeing"])
        return successor.targetState.props["Comm_wellbeing"][self.group_idx]
    
    def StateHeuristic(self, state:State):
        return 0

class Wellbeing(Consideration):
    def __init__(self, group="all", tag='Wellbeing'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0
        self.groupTag = f"{group}:" if group != "all" else ""
        self.neg = NegativeWellbeing(group)


    def judge(self, successor: Successor):
        return self.neg.judge(successor)

    def StateHeuristic(self, state:State):
        return self.neg.StateHeuristic(state) + self.pos.StateHeuristic(state)


class PreferAction(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag="Prefer_Action"
        self.default = 0
    
    def judge(self, successor: Successor):
        if ("go_to" in successor.action or "wait" in successor.action):
            return -1
        return 0

    def StateHeuristic(self, state:State):
        return 0

class Odds:
    Red_None = 0.1
    Red_Hunger = 0
    Red_Injured = 0.2
    Red_Severe = 0.7

    Red_Severe_Success = 0.2
    Red_Treat_Success = 0.2
    Red_Injured_Success = 0.2


    Blue_None = 0.5
    Blue_Hunger = 0
    Blue_Injured = 0.25
    Blue_Severe = 0.25

    Blue_Severe_Success = 0.4
    Blue_Treat_Success = 0.4
    Blue_Injured_Success = 0.4

class SearchRescue(MDP):
    AdjEdge = {}
    Community = []
    WaitEnabled = False
    initialProps = {
        "time":0,
        "holding": 'None',
        "Comm_wellbeing": [],
        "curr_tile": 0,
    }
    
    def __init__(self, Theories, Considerations, initialProps=None, Horizon=5, Budget=5, RequireSearch=False, **kwargs):
        super().__init__()
        
        if initialProps != None:
            SearchRescue.initialProps = deepcopy(initialProps)

        if Horizon != None:
            SearchRescue.initialProps['horizon'] = Horizon
        
        self.RequireSearch = RequireSearch

        self.stateFactory(SearchRescue.initialProps) # Create at least one initial state

        self.rules = [SearchRescue.Move, 
                      SearchRescue.UseBase, 
                      SearchRescue.InteractDisease,
                      SearchRescue.InteractHunger,
                      SearchRescue.InteractSevere,
                      SearchRescue.InteractInjured,
                      SearchRescue.InteractUnknown, 
                      SearchRescue.AdvanceTime] 

        self.budget = Budget
        self.MaxOutsideTime = 3
        self.horizon=Horizon

        self.Theories = []
        self.theorySetup(Theories, Considerations)
        
    @staticmethod
    def GenerateHubSpokeGraph(Communities:list, Spoke_Length:int, active_sites_per_community:int=None):
        if (active_sites_per_community is None):
            active_sites_per_community = Spoke_Length
        hub = 0
        next_node = 1
        adj = {hub: []}
        community = [""]
        true_tile_states = ["base"]
        arms: dict[str, list[int]] = {}
        for com in range(len(Communities)):
            arms[com] = []
            previous = hub

            for depth in range(1, Spoke_Length + 1):
                node = next_node
                next_node += 1

                arms[com].append(node)
                adj[node] = []

                # Bidirectional corridor edge.
                adj[previous].append(node)
                adj[node].append(previous)

                community.append(com)
                true_tile_states.append("none")
                previous = node

        comm_wellbeing = []
        # Assign random needs to sites.
        for com, nodes in arms.items():
            comm_wellbeing.append(0)
            for idx, node in enumerate(nodes):
                if idx >= active_sites_per_community:
                    true_tile_states[node] = "none"
                else:
                    st = random.choice(["injured", "severe", "hunger"])
                    true_tile_states[node] = st
                    if st=="severe":
                        comm_wellbeing[com] -= 3
                    if st=="injured":
                        comm_wellbeing[com] -= 2
                    if st=="hunger":
                        comm_wellbeing[com] -= 1
                    
                    
        SearchRescue.AdjEdge = adj
        SearchRescue.DeterministicTileStates = true_tile_states
        SearchRescue.Community = community

        SearchRescue.initialProps = {
            "time": 0,
            "Comm_wellbeing": comm_wellbeing,
            "Comm_names": Communities,
            "holding": "None",
            "curr_tile": hub,
        }

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
        SearchRescue.initialProps["curr_tile"] = 7

    def isGoal(self, state:State) -> bool:
        # returned/remained at original position.
        return False #state.props['tile_state'][state.props["curr_tile"]] == "base"

    
    def getActions(self, state:State) -> list:
        acts = []
        if (state.props['time'] >= self.horizon):
            return acts
        
        # Add movement actions
        for next_tile in SearchRescue.AdjEdge[state.props["curr_tile"]]:
            acts.append(f"go_to:{next_tile}")
        
        if SearchRescue.WaitEnabled:
            acts.append("wait")

        currTile = state.props["tile_state"][state.props["curr_tile"]]
        holding =  state.props["holding"]
        
        if currTile == "base":
            if state.props['holding'] == "None":
                acts.append("get_medicine")
                acts.append("get_food")
        
        elif currTile=="?":
            acts.append("search")

        elif "hunger" in currTile and holding == "food":
            acts.append("feed")

        elif "disease" in currTile and holding == "medicine":
            acts.append("medicate")
        
        elif "disease" in currTile and holding != "medicine":
            acts.append("treat")

        elif "severe" in currTile:
            acts.append("surgery")

        elif currTile == "injured":
            acts.append("first-aid")
        
        return acts

    def Move(self, props, prob, action:str):
        props_ = deepcopy(props)
        if (action.startswith("go_to")):
            props_["curr_tile"] = int(action.split(":")[1])
        return [(props_, prob)]

    
    # Charging results and surgery results
    def UseBase(self, props, prob, action:str):
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
            props["Comm_wellbeing"][SearchRescue.Community[props['curr_tile']]] += 1
            props["holding"] = "None"
        return [(props, prob)]
    
    def InteractDisease(self, props, prob, action:str):
        if (action != "medicate" and action!= "treat"):
            return [(props, prob)]

        if action=="medicate":
            props_["Comm_wellbeing"][SearchRescue.Community[props_['curr_tile']]] += 2
            return [(props, prob)]

        com = SearchRescue.Community[props["curr_tile"]]
        pr_treat_success = Odds.Red_Treat_Success if com=="red" else Odds.Blue_Treat_Success
        if action != "treat":
            return [(props, prob)]

        o = []
        props_ = deepcopy(props)
        props_["Comm_wellbeing"][SearchRescue.Community[props_['curr_tile']]] += 2
        props_["holding"] = ""
        o.append((props_, prob * pr_treat_success))

        props_ = deepcopy(props)
        props_["holding"] = ""
        o.append((props_, prob * (1 - pr_treat_success)))
        return o
    
    def InteractSevere(self, props, prob, action:str):
        if (action != "surgery"):
            return [(props, prob)]
        
        tileIdx = props["curr_tile"]
        if (props["tile_state"][tileIdx] != "severe"):
            return [(props, prob)]
    
        com = SearchRescue.Community[tileIdx]
        pr_success = Odds.Red_Severe_Success if com=="red" else Odds.Blue_Severe_Success

        outcomes = []
        pr = deepcopy(props)
        pr["Comm_wellbeing"][SearchRescue.Community[pr['curr_tile']]] += 3
        outcomes.append((pr, prob * pr_success))

        pr = deepcopy(props)
        outcomes.append((pr, prob * (1 - pr_success)))

        return outcomes

    def InteractInjured(self, props, prob, action:str):
        if (action != "first-aid"):
            return [(props, prob)]
        
        tileIdx = props["curr_tile"]
        
        com = SearchRescue.Community[tileIdx]
        pr_success = Odds.Red_Injured_Success if com=="red" else Odds.Blue_Injured_Success

        outcomes = []
        pr = deepcopy(props)
        pr["tile_state"][tileIdx] = "helped"
        outcomes.append((pr, prob * pr_success))

        pr = deepcopy(props)
        pr["tile_state"][tileIdx] = "failed"
        outcomes.append((pr, prob * (1 - pr_success)))

        return outcomes

    def InteractUnknown(self, props, prob, action:str):
        tileIdx = props["curr_tile"]

        if (props["tile_state"][tileIdx] != "?"):
            return [(props, prob)]
        
        if (action != "search" and self.RequireSearch):
            return [(props, prob)]
        
        com = SearchRescue.Community[tileIdx]
        outcomes = []
        if com=="red":
            props_ = deepcopy(props)
            props_["tile_state"][tileIdx] = "none"
            outcomes.append((props_, prob * Odds.Red_None))

            props_ = deepcopy(props)
            props_["tile_state"][tileIdx] = "injured"
            outcomes.append((props_, prob * Odds.Red_Injured))

            #props_ = deepcopy(props)
            #props_["tile_state"][tileIdx] = "hunger"
            #outcomes.append((props_, prob * Odds.Red_Hunger))

            props_ = deepcopy(props)
            props_["tile_state"][tileIdx] = "severe"
            outcomes.append((props_, prob * Odds.Red_Severe))

        if com=="blue":
            props_ = deepcopy(props)
            props_["tile_state"][tileIdx] = "none"
            outcomes.append((props_, prob * Odds.Blue_None))

            props_ = deepcopy(props)
            props_["tile_state"][tileIdx] = "injured"
            outcomes.append((props_, prob * Odds.Blue_Injured))

            #props_ = deepcopy(props)
            #props_["tile_state"][tileIdx] = "hunger"
            #outcomes.append((props_, prob * Odds.Blue_Hunger))

            props_ = deepcopy(props)
            props_["tile_state"][tileIdx] = "severe"
            outcomes.append((props_, prob * Odds.Blue_Severe))

        return outcomes

    # time advances each transition
    def AdvanceTime(self, props, prob, action):
        props_ = deepcopy(props)
        props_["time"] = props_["time"] + 1
        return [(props_, prob)]
    

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
                mc = Wellbeing(group='all', tag=tag)
            
            elif 'Cost'==tag:
                mc = Cost(deadline=self.horizon)
            elif 'FindInfo'==tag:
                mc = FindInfo()
            elif 'NeverIgnore'==tag:
                mc = NeverIgnore()
            elif 'Prefer_Action'==tag:
                mc = PreferAction()
            else:
                raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
            mc.componentOf = c["Component_of"]
            self.Considerations.append(mc)