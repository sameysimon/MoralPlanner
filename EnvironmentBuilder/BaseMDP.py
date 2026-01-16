from abc import ABC, abstractmethod 
from copy import deepcopy

class ConsiderationTypes():
    UTILITY="Utility"
    ABSOLUTISM="Absolutism"
    MAXIMIN="Maximin"

class State:
    def __init__(self, _index=0, _props={}, _info={}):
        self.id = _index
        self.actions:list[str] = []
        self.props:dict = _props
        self.info:dict = _info
        self.successors = {}
        self.ancestors = []
        
class Successor:
    def __init__(self, _sourceState:State, _targetState:State, _probability:float, _action:str):
        self.sourceState:State = _sourceState
        self.targetState:State = _targetState
        self.probability:float = _probability
        self.action:str = _action

class MDP(ABC):
    def __init__(self):
        self.states:list[State] = [] # List of explicit instantiated states
        self.rules = [] # List of rule functions for state-action transitions
        self.__statePropIndex = {}
        self.__expandedStates = [] # State indices of fully expanded states (has State, successors)
        self.infinite:bool = False
        
        self.TheoryClasses = []
        self.Considerations:list[Consideration]=[]
        self.Theories:list[Theory]=[]

        self.actions:list[str] = []
        self.CostTheory:Consideration = None


    def EmptyValuation(self) -> dict:
        """ Returns estimates with null/default evaluations.
        Returns:
            dict: Expected Ethical Estimate (dictionary : str -> blank estimate)"""
        v = {}
        for t in self.Theories:
            v[t.tag] = t.EmptyEstimate()
        return v
        
    def isGoal(self, state):
        return False

    def getStateHeuristic(self, state: State) -> dict:
        """Returns heuristic value for each moral theory for state. 
        Returns:
            dict: dict: theory.tag -> estimated worth.
        """
        h = {}
        for c in self.Considerations:
            h[c.tag] = c.StateHeuristic(state)
        return h

    def makeAllStatesExplicit(self):
        """Creates explicit graph representation for all states.
        Returns:
            bool: Whether it was successful. Fails if infinite flag is True.
        """
        self.actions = set()
        if self.infinite:
            return False
        idx = 0
        while idx < len(self.states):
            state_actions = self.getActions(self.states[idx])
            for a in state_actions:
                self.actions.add(a)
                self.getActionSuccessors(self.states[idx], a)
            idx+=1
        return True
    
    def getActionSuccessors(self, state: State, action:str, readOnly=False) -> list[Successor]:
        """Returns (or generates if required) successor objects using mdp rules. 

        Args:
            state (State): State.
            action (str): Action.
            readOnly (bool, optional): Will not add to explicit graph/generate new states when True. Defaults to False.

        Returns:
            list: list of Successor objects.
        """
        if action in state.successors.keys():
            return state.successors[action]
        if readOnly:
            return []
        # Create successors since they are not in the dict.
        s = self.__buildSuccessors(state, action)

        return s

    def __buildSuccessors(self, state: State, action:str):
        if (state.id==18 and action=='right'):
            print()
        successorsValues = [(state.props, 1)]
        for ruleFunc in self.rules:
            ruleSuccessorsValues = []
            for existing in successorsValues:
                props = deepcopy(existing[0])
                new = ruleFunc(self, props, existing[1], action)
                if (not isinstance(new, list)):
                    raise Exception(f"Rule func {ruleFunc} does not return list of items from {props}, instead returns {new}")
                if (len(new)==0):
                    raise Exception(f"Rule func {ruleFunc} with properties {props} had no successors.")
                for n in new:
                    if (not isinstance(n, tuple)):
                        raise Exception(f"Rule func {ruleFunc} does not return properties+probability tuple from {props}, instead returns {n}")
                    if (not isinstance(n[0], dict)):
                        raise Exception(f"Rule func {ruleFunc} does not return properties dict from {props}, instead returns {n[0]}")
                    if (not (isinstance(n[1], float) or isinstance(n[1], int))):
                        raise Exception(f"Rule func {ruleFunc} does not return probability from {props}, instead returns {n[1]}")

                ruleSuccessorsValues.extend(new)
            successorsValues = ruleSuccessorsValues
        
        scrToProb = {}
        for targetProperties, probability in successorsValues:
            scrState = self.stateFactory(targetProperties)
            scrToProb.setdefault(scrState.id, 0)
            scrToProb[scrState.id] += probability
        
        state.successors[action] = []
        for scrID, p in scrToProb.items():
            successorState = Successor(state, self.states[scrID], p, action)
            state.successors[action].append(successorState)

        return state.successors[action]

    def stateFactory(self, stateProps) -> State:
        """Generates new State object based of state properties.
        
        Returns:
              New State object, or existing State if already exists with state props
        """
        propStr = str(stateProps)
        if propStr in self.__statePropIndex.keys():
            return self.states[self.__statePropIndex[propStr]]

        index = len(self.states)
        self.__statePropIndex[propStr] = index
        self.states.append(State(_index=index, _props = stateProps))
        return self.states[self.__statePropIndex[propStr]]

    def expandState(self, state:State):
        """Generate successor states for all actions for state."""
        self.__expandedStates.append(state.id)
        oldStateCount = len(self.states)
        actions = self.getActions(state)
        for actionStr in actions:
            successors = self.getActionSuccessors(state, actionStr)
            for successor in successors:
                [self.getActionSuccessors(successor.targetState, a) for a in self.getActions(successor.targetState)]
        
        newStates = []
        for i in range(oldStateCount, len(self.states)):
            newStates.append(self.states[i])
        return newStates

class Theory(ABC):
    def __init__(self):
        super().__init__()
        self.rank=0
        self.name=""
        self.type=""

class Consideration(ABC):
    def __init__(self):
        super().__init__()
        self.componentOf:list[str] = []

    @abstractmethod
    def judge(self, successor:Successor):
        pass
    def heuristic(self, state:State):
        pass

class DefaultCost(Consideration):
    def __init__(self):
        super().__init__()
        self.type='utility'
        self.rank=-1
        self.tag='cost'
    def judge(self, successor:Successor):
        return 0
    def heuristic(self, state:State):
        return 0
