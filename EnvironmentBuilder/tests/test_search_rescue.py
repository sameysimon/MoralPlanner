import unittest
from EnvironmentBuilder.SearchRescue.SearchRescueProblem import SearchRescue

class TestSearchRescue(unittest.TestCase):
    def setUp(self):
        self.mdp = SearchRescue(horizon=5)
        self.mdp.makeAllStatesExplicit()

    def test_initial_state(self):
        initial_state = self.mdp.states[0]
        self.assertEqual(initial_state.props['time'], 0)
        self.assertEqual(initial_state.props['curr_tile'], 0)
        self.assertEqual(len(initial_state.props['tile_state']), 5)

    def test_valid_actions(self):
        initial_state = self.mdp.states[0]
        actions = self.mdp.getActions(initial_state)
        self.assertIn('go_to:1', actions)
        self.assertIn('go_to:3', actions)
        self.assertIn('wait', actions)

    def test_state_transitions(self):
        initial_state = self.mdp.states[0]
        successors = self.mdp.getActionSuccessors(initial_state, 'go_to:1')
        self.assertTrue(len(successors) > 0)
        next_state = successors[0].targetState
        self.assertEqual(next_state.props['curr_tile'], 1)
        self.assertEqual(next_state.props['time'], 1)

        # Check all sum to 1.
        for s in self.mdp.states:
            for a in self.mdp.getActions(s):
                scrs = self.mdp.getActionSuccessors(s, a)
                self.assertEqual(1, sum([scr.probability for scr in scrs]))


if __name__ == '__main__':
    unittest.main()
