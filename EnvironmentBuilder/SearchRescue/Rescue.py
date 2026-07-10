from copy import deepcopy
from EnvironmentBuilder.BaseMDP import *
import numpy as np
import random
import networkx as nx

class Wellbeing(Consideration):
    def __init__(self, mdp, group="all", group_idx=0, tag='Wellbeing'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0
        self.group = group
        self.group_idx = group_idx
        self.mdp = mdp

    def judge(self, successor: Successor):
        u = 0
        if self.group == self.mdp.Community[successor.targetState.props["curr_tile"]] or self.group=="all":
            if successor.action =='heal' and successor.targetState.props["success"]==True:
                u += 1

        if self.group == successor.sourceState.props['holding'] and successor.targetState.props['holding'] == "nothing":
            u += 3

        return u
    
    def StateHeuristic(self, state:State):
        return 0
    

class Searcher(Consideration):
    def __init__(self, group="all", group_idx=0, tag='Searcher'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0
        self.group = group
        self.group_idx = group_idx

    def judge(self, successor: Successor):
        u = 0
        if self.group == successor.sourceState.props['holding'] and successor.targetState.props['holding'] == "nothing":
            u += 3

        return u
    
    def StateHeuristic(self, state:State):
        return 0

class Rescue(MDP):
    AdjEdge = {}
    Community = []
    WaitEnabled = False
    initialProps = {
        "time": 0,
        "curr_tile": 0,
        "success": False,
        "holding": "nothing",
        "tile_type": []
    }
    
    def __init__(self, Theories, Considerations, initialProps=None, Horizon=5, Budget=5, RequireSearch=False,
                  Teams=['red', 'blue'], unknown_depth=1, link_back_dist=4, Odds=None, **kwargs):
        super().__init__()
        
        if initialProps != None:
            self.initialProps = deepcopy(initialProps)

        if Horizon != None:
            self.initialProps['horizon'] = Horizon

        self.BuildMyGraph(Teams, unknown_depth, link_back_dist, Odds)

        
        self.RequireSearch = RequireSearch

        self.stateFactory(self.initialProps) # Create at least one initial state

        self.rules = [Rescue.ResetVars,
                      Rescue.Move, 
                      Rescue.Heal,
                      Rescue.Search,
                      Rescue.Deliver,
                      Rescue.AdvanceTime] 

        self.budget = Budget
        self.horizon=Horizon

        self.Theories = []
        self.theorySetup(Theories, Considerations)
        
    def BuildMyGraph(self, teams=['red', 'blue'], unknown_depth=1, link_back_dist=4, Odds=None):
        self.Teams = teams
        self.Odds = {}
        for t in teams:
            self.Odds[f"{t}_hosp_success"] = 1 / len(teams)
            self.Odds[f"{t}_found"] = 1 / len(teams)
        self.Odds[f"found_person"] = 0.5

        if not Odds is None:
            self.Odds = self.Odds | Odds
        
        self.AdjEdge = {
            0:[]
        }
        self.initialProps["curr_tile"] = 0
        self.initialProps["tile_type"] = ["base"]
        self.Community = [""]
        for i in range(len(teams)):
            self.initialProps["tile_type"].append('hospital')
            self.Community.append(teams[i])
            self.AdjEdge[i+1] = [0]
            self.AdjEdge[0].append(i+1)
        unknown_start = len(teams) + 1
        if unknown_depth>0:
            self.AdjEdge[0].append(unknown_start)
            self.AdjEdge[unknown_start] = [0]
            self.initialProps["tile_type"].append("?")
            self.Community.append("")      

        for i in range(unknown_start + 1 , unknown_depth + unknown_start):
            self.AdjEdge[i] = [i-1]
            self.AdjEdge[i-1].append(i)
            if (i % link_back_dist ==0):
                self.AdjEdge[i].append(0)
                self.AdjEdge[0].append(i)
            self.initialProps["tile_type"].append("?")
            self.Community.append("")


        

    def isGoal(self, state:State) -> bool:
        return False

    
    def getActions(self, state:State) -> list:
        acts = []
        if (state.props['time'] >= self.horizon):
            return acts
        
        # Add movement actions
        for next_tile in self.AdjEdge[state.props["curr_tile"]]:
            acts.append(f"go_to:{next_tile}")
        
        currTile = state.props['tile_type'][state.props["curr_tile"]]
        
        if currTile == "hospital":
            acts.append('heal')
        
        return acts

    def ResetVars(self, props, prob, action:str):
        props['success'] = False
        return [(props, prob)]

    def Move(self, props, prob, action:str):
        props_ = deepcopy(props)
        if (action.startswith("go_to")):
            props_["curr_tile"] = int(action.split(":")[1])
        return [(props_, prob)]

    # Charging results and surgery results
    def Heal(self, props, prob, action:str):
        if action != "heal":
            return [(props, prob)]
        curr_comm =  self.Community[props['curr_tile']]
        pr_success = self.Odds[f"{curr_comm}_hosp_success"]
        outs = []
        props_ = deepcopy(props)
        props_['success']=True
        outs.append((props_, prob*pr_success))
        props_ = deepcopy(props)
        props_['success']=False
        outs.append((props_, prob*(1 - pr_success)))
        return outs
    
    def Search(self, props, prob, action:str):
        curr_type = props['tile_type'][props['curr_tile']]
        if (curr_type != "?"):
            return [(props, prob)]
        if (props['holding'] != "nothing"):
            return [(props, prob)]

        o = []
        props['tile_type'][props['curr_tile']] = ""
        props_ = deepcopy(props)
        o.append((props_, prob * (1 - self.Odds['found_person'])))
        for t in self.Teams:
            props_ = deepcopy(props)
            props_['holding'] = t
            o.append((props_, prob * self.Odds['found_person'] * self.Odds[f"{t}_found"]))
        return o

    def Deliver(self, props, prob, action:str):
        curr_type = props['tile_type'][props['curr_tile']]
        if (curr_type != "hospital" and curr_type != "base"):
            return [(props, prob)]
        if (curr_type == "hospital" and props['holding'] != self.Community[props['curr_tile']]):
            return [(props, prob)]
        props["holding"] = "nothing"
        return [(props, prob)]

        
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
            if ":wellbeing" in tag:
                g = tag.split(':')[0]
                if len(g)==0:
                    g='all'
                mc = Wellbeing(self, group=g, tag=tag)
            elif ":search" in tag:
                g = tag.split(':')[0]
                if len(g)==0:
                    g='all'
                mc = Wellbeing(self, group=g, tag=tag)
            else:
                raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
            mc.componentOf = c["Component_of"]
            self.Considerations.append(mc)