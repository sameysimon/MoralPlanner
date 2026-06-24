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
    
class PositiveWellbeing(Consideration):
    def __init__(self, group="all", tag='Positive_Well-being'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0
        self.group = group

    def judge(self, successor: Successor):
        u = 0
        src_props = successor.sourceState.props
        tar_props = successor.targetState.props

        for i in range(len(src_props["tile_state"])):
            if (SearchRescue.Community[i] != self.group and self.group!='all'):
                continue
            if src_props["tile_state"][i] == "hunger" and tar_props["tile_state"][i] == "helped":
                u += 2
            if src_props["tile_state"][i] == "injured" and tar_props["tile_state"][i] == "helped":
                u += 3
            if src_props["tile_state"][i] == "severe" and tar_props["tile_state"][i] == "helped":
                u += 4
        return u
    
    def StateHeuristic(self, state:State):
        return 500

class NegativeWellbeing(Consideration):
    def __init__(self, group="all", tag='Negative_Well-being'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0
        self.group = group

    def judge(self, successor: Successor):
        u = 0
        src_props = successor.sourceState.props
        tar_props = successor.targetState.props

        for i in range(len(src_props["tile_state"])):
            if (SearchRescue.Community[i] != self.group and self.group!='all'):
                continue
            if tar_props["tile_state"][i] ==  f"hunger":
                u -= 1

            if tar_props["tile_state"][i] ==  f"injured":
                u -= 2
            
            if tar_props["tile_state"][i] ==  f"severe":
                u -= 3
        return u
    
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
        self.pos = PositiveWellbeing(group)


    def judge(self, successor: Successor):
        return self.neg.judge(successor) + self.pos.judge(successor)

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

    initialProps = {
        "time":0,
        "tile_state": [], # status of current tile
        "holding": 'None',
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
    def GenerateGraph(
        nodes: int = 5,
        edges: int | None = None,
        red_nodes: int | None = None,
        blue_nodes: int | None = None,
        base_nodes: int = 1,
        seed: int | None = None):
       
        if nodes <= 0:
            raise ValueError("nodes must be at least 1")

        if base_nodes < 1:
            raise ValueError("base_nodes must be at least 1")

        if base_nodes > nodes:
            raise ValueError("base_nodes cannot be greater than nodes")

        non_base_nodes = nodes - base_nodes

        # Work out the red/blue split when one or both values are omitted.
        if red_nodes is None and blue_nodes is None:
            red_nodes = non_base_nodes // 2
            blue_nodes = non_base_nodes - red_nodes

        elif red_nodes is None:
            red_nodes = non_base_nodes - blue_nodes

        elif blue_nodes is None:
            blue_nodes = non_base_nodes - red_nodes

        if red_nodes < 0 or blue_nodes < 0:
            raise ValueError("red_nodes and blue_nodes cannot be negative")

        if red_nodes + blue_nodes + base_nodes != nodes:
            raise ValueError(
                "red_nodes + blue_nodes + base_nodes must equal nodes"
            )

        # A strongly connected directed graph needs at least one directed
        # cycle containing every node.
        if nodes == 1:
            minimum_edges = 0
            maximum_edges = 0
        else:
            minimum_edges = nodes
            maximum_edges = nodes * (nodes - 1)

        # Default to about two outgoing edges per node.
        if edges is None:
            edges = min(maximum_edges, max(minimum_edges, nodes * 2))

        if not minimum_edges <= edges <= maximum_edges:
            raise ValueError(
                f"For {nodes} nodes, edges must be between "
                f"{minimum_edges} and {maximum_edges} to produce a "
                "strongly connected directed graph without self-loops."
            )

        rng = random.Random(seed)
        node_ids = list(range(nodes))

        # --------------------------------------------------------------
        # Assign base, red, and blue nodes
        # --------------------------------------------------------------

        shuffled_nodes = node_ids.copy()
        rng.shuffle(shuffled_nodes)

        base_set = set(shuffled_nodes[:base_nodes])

        remaining_nodes = shuffled_nodes[base_nodes:]
        red_set = set(remaining_nodes[:red_nodes])
        blue_set = set(remaining_nodes[red_nodes:])

        # Reset the arrays rather than appending to old values.
        SearchRescue.Community = [
            "" if node in base_set
            else "red" if node in red_set
            else "blue"
            for node in node_ids
        ]

        SearchRescue.initialProps["tile_state"] = [
            "base" if node in base_set else "?"
            for node in node_ids
        ]

        # The agent must start at one of the bases.
        SearchRescue.initialProps["curr_tile"] = min(base_set)

        # --------------------------------------------------------------
        # Generate the graph
        # --------------------------------------------------------------

        graph = nx.DiGraph()
        graph.add_nodes_from(node_ids)

        if nodes > 1:
            # Start with a randomly ordered directed cycle. This guarantees
            # that every node can reach every other node.
            cycle = node_ids.copy()
            rng.shuffle(cycle)

            cycle_edges = [
                (cycle[i], cycle[(i + 1) % nodes])
                for i in range(nodes)
            ]

            graph.add_edges_from(cycle_edges)

        # Find every valid edge not already used by the cycle.
        candidate_edges = [
            (source, target)
            for source in node_ids
            for target in node_ids
            if source != target
            and not graph.has_edge(source, target)
        ]

        rng.shuffle(candidate_edges)

        number_to_add = edges - graph.number_of_edges()
        graph.add_edges_from(candidate_edges[:number_to_add])

        # Convert the NetworkX graph into your existing adjacency-list format.
        SearchRescue.AdjEdge = {
            node: sorted(graph.successors(node))
            for node in node_ids
        }

        return SearchRescue.AdjEdge
    
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
        return state.props['tile_state'][state.props["curr_tile"]] == "base"

    
    def getActions(self, state:State) -> list:
        acts = []
        if (state.props['time'] >= self.horizon):
            return acts

        #
        # Add movement actions
        #
        for next_tile in SearchRescue.AdjEdge[state.props["curr_tile"]]:
            acts.append(f"go_to:{next_tile}")
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
            props["tile_state"][props["curr_tile"]] = "helped"
            props["holding"] = ""
        return [(props, prob)]
    
    def InteractDisease(self, props, prob, action:str):
        if (action != "medicate" and action!= "treat"):
            return [(props, prob)]

        if action=="medicate":
            props["tile_state"][props["curr_tile"]] = "helped"
            return [(props, prob)]

        com = SearchRescue.Community[props["curr_tile"]]
        pr_treat_success = Odds.Red_Treat_Success if com=="red" else Odds.Blue_Treat_Success
        if action != "treat":
            return [(props, prob)]

        o = []
        props_ = deepcopy(props)
        props_["tile_state"][props_["curr_tile"]] = "helped"
        props_["holding"] = ""
        o.append((props_, prob * pr_treat_success))

        props_ = deepcopy(props)
        props_["tile_state"][props_["curr_tile"]] = "failed"
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
        pr["tile_state"][tileIdx] = "helped"
        outcomes.append((pr, prob * pr_success))

        pr = deepcopy(props)
        pr["tile_state"][tileIdx] = "failed"
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
            
            elif 'red:pos_wellbeing'==tag:
                mc = PositiveWellbeing(group='red', tag=tag)
            elif 'blue:pos_wellbeing'==tag:
                mc = PositiveWellbeing(group='blue', tag=tag)

            elif 'red:neg_wellbeing'==tag:
                mc = NegativeWellbeing(group='red', tag=tag)
            elif 'blue:neg_wellbeing'==tag:
                mc = NegativeWellbeing(group='blue', tag=tag)

            elif 'pos_wellbeing'==tag:
                mc = PositiveWellbeing(tag=tag, group="all")
            elif 'neg_wellbeing'==tag:
                mc = NegativeWellbeing(tag=tag, group="all")

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