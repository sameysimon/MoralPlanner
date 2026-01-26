from copy import deepcopy
from EnvironmentBuilder.BaseMDP import *


time = "time"
myPatientPos = "myPatientPos"
myPatientPreCond = "myPatientPreCond"
myPatientGivesUp = "myPatientGivesUp"
timesAskedMyPatient = "timesAskedMyPatient"
myPatientDone = "myPatientDone"
myPos = "myPos"
terminated = "terminated"
calledStaff = "calledStaff"
myPatientNeedsHelp = "myPatientInHarm"
staffAppear = "staffAppear"
friendInformedConsent = "friendInformedConsent"
friendRefusesConsent = "friendRefusesConsent"

class StaffTime(Consideration):
    def __init__(self, tag='StaffTime'):
        super().__init__()
        self.type='Cost'
        self.rank=0
        self.tag=tag
        self.default = False

    def judge(self, successor: Successor) -> float:
        if (successor.targetState.props["staffAppear"]):
            return -10
        if (successor.targetState.props["calledStaff"]):
            return -5
        # amortized + energy cost of the robot.
        return -1
        
    def StateHeuristic(self, state:State):
        return 0

class Privacy(Consideration):
    def __init__(self, tag='Privacy'):
        super().__init__()
        self.type='Absolutism'
        self.rank=0
        self.tag=tag
        self.default = False

    def judge(self, successor: Successor):
        if (successor.targetState.props[myPos] == "InRoom" and not successor.targetState.props[friendInformedConsent]):
            return True
        return False
        
    def StateHeuristic(self, state:State):
        return False

class Autonomy(Consideration):
    def __init__(self, tag='Autonomy'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.action == "Block"):
            return -5
        if (successor.action == "Ask"):
            return -1 * (successor.targetState.props[timesAskedMyPatient] + 1)
        if (successor.action == "Suggest"):
            return -1
        return 0
        
    def StateHeuristic(self, state:State):
        return 0
    
class OrdinalAutonomy(Consideration):
    # 0 is neutral;   1 is good;   -1 minor;  -2 bad;   -3 severe!
    def __init__(self, optimalityType, tag='OrdinalAutonomy'):
        super().__init__()
        self.type='Ordinal'
        self.rank=0
        self.optimalityType=optimalityType
        self.tag=tag
        self.default = 0
        self.ordinalLabels = {"0": "Neutral", "1": "Respectful", "-1": "Minor", "-2": "Significant", "-1": "Severe"}

    def judge(self, successor: Successor):
        if (successor.action == "Block"):
            return -3
        if (successor.action == "Ask"):
            return -1
        if (successor.action == "Suggest"):
            return -1
        return 0
        
    def StateHeuristic(self, state:State):
        return 0

class Deception(Consideration):
    def __init__(self, tag='Deception'):
        super().__init__()
        self.type='Absolutism'
        self.rank=0
        self.tag=tag
        self.default = False

    def judge(self, successor: Successor):
        if (successor.action=="Suggest"):
            return True
        return False
        
    def StateHeuristic(self, state:State):
        return False

class Health(Consideration):
    def __init__(self, tag='Health'):
        super().__init__()
        self.type='Utility'
        self.rank=0
        self.tag=tag
        self.default = 0

    def judge(self, successor: Successor):
        if (successor.targetState.props["myPatientInHarm"]):
            return -5
        return 0
        
    def StateHeuristic(self, state:State):
        return 0


class Elder(MDP):
    defaultProps = {
        time: 0, # Can be upto horizon=7
        terminated: False,
        myPatientPos: "Outside", # Can be "InRoom"
        myPos: "Outside",
        myPatientPreCond: 1, # Can be upto 5 pre-existing conditions.
        myPatientGivesUp: False,
        timesAskedMyPatient: 0, # How many times the patient has been asked to stay. RESET to 0 if goes inside.
        myPatientDone : False, # Whether the patient is satisfied.
        calledStaff: False,
        myPatientNeedsHelp: False,
        staffAppear: False,
        friendInformedConsent: False,
        friendRefusesConsent: False
    }
    # Measures to stop the patient from going into the room.
    outsideActions = ["Ask", "Suggest", "Block", "Allow", "CallStaff", "Permission"]
    # Actions once my patient is already in the room.
    inRoomActions = ["Wait", "CallStaff", "EnterRoom", "Permission"]
    # Base Probabilities
    Pr_PatientGivesUpWithBlock = 0.3
    Pr_PatientTakesSuggestion = 0.4
    Pr_PatientCompliesWithAsk = 0.3
    Pr_FriendGivesConsent = 0.6
    Pr_StaffArrive = 0.5
    Pr_HarmBase = 0.1


    def __init__(self, Theories, Considerations, initialProps=None, horizon=5, Budget=4, **kwargs):
        super().__init__()
        if initialProps==None:
            initialProps=Elder.defaultProps
        if not horizon==None:
            initialProps['horizon'] = horizon
        self.stateFactory(initialProps) # Create at least one initial state
        # Order intentional. After going inside room on an epoch (from outside room rule),
        # want to only process inside room next epoch, so put InsideRoom rule first.
        self.rules = [Elder.PatientHarm, Elder.DoStaffArrive, Elder.InsideRoom, Elder.PatientOutsideRoom, Elder.ResetVars, Elder.AdvanceTime] 

        #self.CostTheory = Time()
        self.Theories = []
        self.theorySetup(Theories, Considerations)
        
        self.budget = Budget
        self.MaxOutsideTime = 3
        self.horizon=horizon


    def getActions(self, state):
        if not(state.props['time'] <= self.horizon-1):
            return []
        acts = ["N/A"]
        if state.props[terminated]:
            return acts
        
        # Stage 1. Patient outside and wants to enter.
        if state.props[myPatientPos]=="Outside" and not (state.props[myPatientDone] or state.props[myPatientGivesUp]):
            # Can take all the measures that have not already been taken.
            acts = deepcopy(Elder.outsideActions)
        # Stage 2. Patient inside the room and robot outside
        if state.props[myPatientPos]=="InRoom" and state.props[myPos]=="Outside":
            # Cannot call staff twice.
            acts = Elder.inRoomActions

        if (state.props[calledStaff] and "CallStaff" in acts):
            acts.remove("CallStaff")
        if ((state.props[friendRefusesConsent] or state.props[friendInformedConsent]) and "Permission" in acts):
            acts.remove("Permission")
        
        return acts


    # If patient in the room and the robot is outside the room, there is a probability that the patient comes to harm.
    def PatientHarm(self, props, prob, action):
        if (props[myPatientPos]=="InRoom" and (props[myPos]!="InRoom" and not props[terminated])):
            outs = []
            # If patient needs help
            props_ = deepcopy(props)
            props_[myPatientNeedsHelp] = True
            pr = prob
            pr *= Elder.Pr_HarmBase * props[myPatientPreCond]
            outs.append((props_, pr ))
            # If nothing happens
            props_ = deepcopy(props)
            pr = 1 - pr
            outs.append((props_, pr ))
            return outs
        
        return [(deepcopy(props), prob)]

    # If robot has called the staff, probability they show up. Probability they do not.
    def DoStaffArrive(self, props, prob, action) -> list:
        if (props[terminated]):
            return [(deepcopy(props), prob)]
        if (not props[calledStaff]):
            return [(deepcopy(props), prob)]

        outs = []
        # Staff come
        props_ = deepcopy(props)
        props_[terminated] = True
        props_[staffAppear] = True
        outs.append((props_, prob * Elder.Pr_StaffArrive))
        # Staff do NOT come
        props_ = deepcopy(props)
        outs.append((props_, prob * (1 - Elder.Pr_StaffArrive)))
        return outs
        

    # Same condition as PatientHarm. Patient in room; robot outside.
    # Robot can enter room, wait, or call staff.
    def InsideRoom(self, props, prob, action) -> list:
        # Only applies if my patient is in room and robot is not.
        if (props[myPatientPos] != "InRoom" or props[terminated]):
            return [(deepcopy(props), prob)]
        
        if (props[myPos]=="InRoom"):
            return [(deepcopy(props), prob)]

        if (action == "EnterRoom"):
            props_ = deepcopy(props)
            props_[myPos] = "InRoom"
            props_[terminated] = True # Scenario ends when robot enters room.
            return [(props_, prob)]

        if (action == "Wait"):
            # If staff are not coming, nothing happens when waiting.
            return [(deepcopy(props), prob)]    

        if (action == "CallStaff" and props[calledStaff]==False):
            props_ = deepcopy(props)
            props_[calledStaff] = True
            return [(props_, prob)]

        if (action=="Permission"):
            outs = []
            props_ = deepcopy(props)
            props_[friendInformedConsent] = True
            outs.append((props_, prob * Elder.Pr_FriendGivesConsent))
            props_ = deepcopy(props)

            props_[friendRefusesConsent] = True
            props_[friendInformedConsent] = False
            outs.append((props_, prob * (1 - Elder.Pr_FriendGivesConsent)))
            return outs


    # If patient is outside the room (with robot):
    # - robot can block entry w/ chance patient gives up.
    # - robot can suggest other action, succeeding w/ some probability.
    # - robot can ask patient not to enter, succeeding w/ some probability.
    def PatientOutsideRoom(self, props, prob, action):
        if (props[myPatientPos]!="Outside" or props[terminated]):
            return [(deepcopy(props), prob)]

        if (action=="N/A"):
            return [(deepcopy(props), prob)]

        if (action=="Allow"):
            props_ = deepcopy(props)
            props_[myPatientPos] = "InRoom"
            return [(props_, prob)]
        
        
        outs = []
        if (action=="Block"):
            # If exceeding time bound, patient gives up.
            if (props["time"] >= self.MaxOutsideTime):
                props_ = deepcopy(props)
                props_[myPatientGivesUp] = True
                return [(props_, prob)]
            
            # If not exceeded max time outside the room, 0.3 chance patient gives up at suggestion
            outs = []
            props_ = deepcopy(props)
            props_[myPatientGivesUp] = True
            outs.append((props_, prob * Elder.Pr_PatientGivesUpWithBlock))

            props_ = deepcopy(props)
            outs.append((props_, prob * (1 - Elder.Pr_PatientGivesUpWithBlock)))
            return outs
        
        if (action=="Suggest"):
            outs = []
            # Wins suggest
            successProb = Elder.Pr_PatientTakesSuggestion
            props_ = deepcopy(props)
            props_[myPatientDone] = True
            outs.append((props_, prob * successProb))
            # Fails persuade
            props_ = deepcopy(props)
            props_[myPatientPos] = "InRoom"
            failProb = 1 - successProb
            outs.append((props_, prob * failProb))
            return outs

        if (action=="Ask"):
            # Wins persuade
            # 0.3(=OldMDP.askedMyPatient) first ask; 0.3/2 = 0.15 second ask; 0.3/3=0.1 third ask.
            props_ = deepcopy(props)
            successProb = Elder.Pr_PatientCompliesWithAsk / (props_[timesAskedMyPatient]+1)
            props_[myPatientDone] = True
            outs.append((props_, prob * successProb))
            # Fails persuade
            props_ = deepcopy(props)
            props_[timesAskedMyPatient] += 1
            props_[myPatientPos] = "InRoom"
            failProb = 1 - successProb
            outs.append((props_, prob * failProb))
            return outs
        
        if (action=="Permission"):
            props_ = deepcopy(props)
            props_[friendInformedConsent] = True
            outs.append((props_, prob * Elder.Pr_FriendGivesConsent))

            props_ = deepcopy(props)
            props_[friendRefusesConsent] = True
            props_[friendInformedConsent] = False
            outs.append((props_, prob * (1 - Elder.Pr_FriendGivesConsent)))
            return outs
        
        if (action=="CallStaff"):
            props_ = deepcopy(props)
            props_[calledStaff] = True
            return [(props_, prob)]
            return outs
        
        return [(props, prob)]

    # Reduces state-space, if robot in room, variables for outside room set to 0. 
    def ResetVars(self, props, prob, action):
        props_ = deepcopy(props)
        if (props_[myPatientPos]=="InRoom"):
            props_[timesAskedMyPatient] = 0
        return [(props_, prob)]



    # time advances each transition
    def AdvanceTime(self, props, prob, action):
        props_ = deepcopy(props)
        props_[time] = props_[time] + 1
        return [(props_, prob)]
    

    def isGoal(self, state):
        return state.props[terminated] or state.props[myPatientDone]

    # Setup stuff.
    def stateString(self, state) -> str:
        return str(state.props)
        
    def theorySetup(self, theoryClasses):
        rank = 0
        mt = 0
        theoryid = 0
        for theoryGroup in theoryClasses:
            for tag in theoryGroup:
                if (not tag in Elder.TheoryClasses.keys()):
                    raise Exception(f"No moral theory with tag '{tag}' for domain 'Elder'.")
                mt = Elder.TheoryClasses[tag]()
                mt.rank = rank
                self.Theories.append(mt)
                theoryid+=1
            rank+=1

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
            if 'privacy'==tag:
                mc = Privacy()
            elif 'autonomy'==tag:
                mc = Autonomy()
            elif 'ordinal_autonomy_optimist'==tag:
                mc = OrdinalAutonomy(1)
            elif 'ordinal_autonomy_pessimist'==tag:
                mc = OrdinalAutonomy(0)
            elif 'health'==tag:
                mc = Health()
            elif 'cost'==tag:
                mc=StaffTime()
            else:
                raise Exception('Moral theory with tag ' + tag + ' at rank ' + str(rank) + ' invalid.')
            mc.componentOf = c["Component_of"]
            self.Considerations.append(mc)

