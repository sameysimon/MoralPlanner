from abc import ABC, abstractmethod 
from copy import deepcopy


class MDP(ABC):
    class State:
        def __init__(self, _index=0, _props={}):
            self.id = _index
            self.actions = []
            self.props = _props
            self.successors = {}
            self.ancestors = []
            
    class Successor:
        def __init__(self, _sourceState, _targetState, _probability, _action):
            self.sourceState = _sourceState
            self.targetState = _targetState
            self.probability = _probability
            self.action = _action

    def __init__(self):
        self.states = [] # List of explicit instantiated states
        self.rules = [] # List of rule functions for state-action transitions
        self.__statePropIndex = {}
        self.__expandedStates = [] # State indices of fully expanded states (has State, successors)
        self.infinite = False
        self.TheoryClasses = []
        self.Theories=[]
        self.actions = []
        self.CostTheory=None


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
        for t in self.Theories:
            h[t.tag] = t.StateHeuristic(state)
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
    
    def getActionSuccessors(self, state: State, action:str, readOnly=False) -> list:
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
        successorsValues = [(state.props, 1)]
        for ruleFunc in self.rules:
            ruleSuccessorsValues = []
            for existing in successorsValues:
                new = ruleFunc(self, existing[0], existing[1], action)
                ruleSuccessorsValues.extend(new)
            successorsValues = ruleSuccessorsValues
        
        scrToProb = {}
        for targetProperties, probability in successorsValues:
            scrState = self.stateFactory(targetProperties)
            scrToProb.setdefault(scrState.id, 0)
            scrToProb[scrState.id] += probability
        
        state.successors[action] = []
        for scrID, p in scrToProb.items():
            successorState = MDP.Successor(state, self.states[scrID], p, action)
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
        self.states.append(MDP.State(_index=index, _props = stateProps))
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


class MoralTheory(ABC):
    @abstractmethod
    def judge(self, successor:MDP.Successor):
        pass
    def heuristic(self, state:MDP.State):
        pass

class DefaultCost(MoralTheory):
    def __init__(self):
        self.type='utility'
        self.rank=-1
        self.tag='cost'
    def judge(self, successor:MDP.Successor):
        return 0
    def heuristic(self, state:MDP.State):
        return 0
