from EnvironmentBuilder.BaseMDP import MDP, MoralTheory



class WellBeing(MoralTheory):
    def __init__(self):
        self.type='utility'
        self.rank=0
        self.tag='wellbeing'
        self.default=0
    # Utility in decreasing stress.
    def judge(self, successor: MDP.Successor):
        oldProps = successor.sourceState.props
        props = successor.targetState.props
        strDelta = 0
        for i in range(props['totalStudents']):
            d = (oldProps['stress'][i] - props['stress'][i])
            if d>0 or (d < 0 and props['stress'][i] >=3):    
                strDelta+= d
        return strDelta
    def StateHeuristic(self, state:MDP.State):
        # No possible stress relief if 0 sessions
        totalWeeks = state.props['totalWeeks'] - state.props['week']
        studentsThisWeek = (state.props['totalStudents']- state.props['student'])
        totalSessions = (totalWeeks*state.props['totalStudents']) + studentsThisWeek
        if totalSessions==0:
            return 0
        # Heuristic is max number of stress decrements per student.
        util = 0
        for i in range(0, state.props['totalStudents']):
            sessions = totalWeeks+1 if i>= state.props['student'] else totalWeeks
            distToMax = state.props['maxStress'] - state.props['stress'][i]
            util += min(sessions, distToMax)*0.7
        # Looking for the average number of grade ups across students.
        h = util
        return h
    
class Education(MoralTheory):
    def __init__(self):
        self.type='utility'
        self.rank=0
        self.tag='education'
        self.default=0
    # Utility in decreasing stress.
    def judge(self, successor: MDP.Successor):
        oldProps = successor.sourceState.props
        props = successor.targetState.props
        avgGrade = 0
        for i in range(props['totalStudents']):
            avgGrade+=(props['grades'][i] - oldProps['grades'][i])

        return avgGrade

    def StateHeuristic(self, state:MDP.State):
        # No possible Grade ups if 0 sessions
        totalWeeks = state.props['totalWeeks'] - state.props['week']
        studentsThisWeek = (state.props['totalStudents']- state.props['student'])
        totalSessions = (totalWeeks*state.props['totalStudents']) + studentsThisWeek
        if totalSessions==0:
            return 0
        # Heuristic is max number of average possible grade ups per student.
        gradeUps = 0
        for i in range(0, state.props['totalStudents']):
            sessions = totalWeeks+1 if i>= state.props['student'] else totalWeeks
            distToMax = state.props['maxGrade'] - state.props['grades'][i]
            gradeUps += min(sessions, distToMax)*0.4
            #gradeUps += state.props['standardChances'][i]
        # Looking for the average number of grade ups across students.
        h = gradeUps
        return h


class NoLies(MoralTheory):
    def __init__(self):
        self.type="utility"
        self.rank=0
        self.tag = "no_lies"
        self.default=False
    def judge(self, successor: MDP.Successor):
        # if action is to lie compare, judge transition bad.
        if successor.action=='neg_lie' or successor.action=='pos_lie':
            return True
        return False
    def StateHeuristic(self, state:MDP.State):
        return False