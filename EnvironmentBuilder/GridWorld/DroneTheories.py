from EnvironmentBuilder.BaseMDP import MDP, Consideration, State, Successor

class Time(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Cost'
        self.rank=1
        self.tag='time'
        self.default = 0

    def judge(self, successor: Successor):
        return -1

    # Simple hamming distance heuristic.
    def StateHeuristic(self, state:State):
        minDist = 99999
        xy = state.props['xy']
        for g in state.props['goals']:
            dist = abs(g[0] - xy[0]) + abs(g[1] - xy[1])
            if (dist < minDist):
                minDist = dist

        return -1 * minDist

class MoralTime(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Utility'
        self.rank=1
        self.tag='time'
        self.default = 0

    def judge(self, successor: Successor):
        xy = successor.targetState.props['xy']
        for g in successor.targetState.props['goals']:
            if g[0]==xy[0] and g[1]==xy[1]:
                return 10
        return -1

    # Simple hamming distance heuristic.
    def StateHeuristic(self, state:State):
        minDist = 99999
        xy = state.props['xy']
        for g in state.props['goals']:
            dist = abs(g[0] - xy[0]) + abs(g[1] - xy[1])
            if (dist < minDist):
                minDist = dist

        return -1 * minDist

class AvoidPlaygrounds(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Threshold'
        self.rank=1
        self.tag='avoid_playgrounds'
        self.threshold=0.95
        self.default = 0

    def judge(self, successor: Successor):
        xy = successor.targetState.props['xy']
        for pl in successor.targetState.props['playgrounds']:
            if (pl[0]==xy[0] and pl[1]==xy[1]):
                return -1
        return 0


    # Simple hamming distance heuristic.
    def StateHeuristic(self, state:State):
        if len(state.props['playgrounds'])==0:
            return 0

        h = 0
        xy = state.props['xy']
        for pl in state.props['playgrounds']:
            d = abs(pl[0] - xy[0]) + abs(pl[1] - xy[1])
            
            if (d==0):
                h+= 1
            else:
                h += (1/(d))
            
            
        # Turned negative.
        return h*-1

class AvoidCheckpoints(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Threshold'
        self.rank=1
        self.tag='avoid_checkpoints'
        self.threshold=0.9
        self.default = 0

    def judge(self, successor: Successor):
        xy = successor.targetState.props['xy']
        for pl in successor.targetState.props['checkpoints']:
            if (pl[0]==xy[0] and pl[1]==xy[1]):
                return -1
        return 0


    # Simple hamming distance heuristic.
    def StateHeuristic(self, state:State):
        maxDist = 0
        xy = state.props['xy']
        for ch in state.props['checkpoints']:
            dist = abs(ch[0] - xy[0]) + abs(ch[1] - xy[1])
            if (dist > maxDist):
                maxDist = dist
        
        # Adds flat entire board traversal to ensure admissability.
        maxDist+= (state.props['max_xy'][0] * state.props['max_xy'][1])
        # Turned negative. 0.3 probability of wrong direction at each point.
        return -0.3 * maxDist


class Time(Consideration):
    def __init__(self):
        super().__init__()
        self.type='Cost'
        self.rank=1
        self.tag='time'
        self.default = 0

    def judge(self, successor: Successor):
        return -1

    # Simple hamming distance heuristic.
    def StateHeuristic(self, state:State):
        return -10