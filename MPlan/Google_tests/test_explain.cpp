//
// Created by Simon Kolker on 17/06/2025.
//
#include "TestBase.hpp"
#include <Solver.hpp>
#include "Logger.hpp"

using namespace std;

class ExplainTest : public TestBase {

};

TEST_F(ExplainTest, BasicSecondSearch) {
    Log::setLogLevel(Trace);
    Runner runner = Runner("SecondSearch.json");
    vector<string> actions = {"A", "B"};
    runner.solve();
    ASSERT_EQ(runner.policies.size(), 1) << "After initial search, should be one dominant policy";

    // Only policy
    auto factPolicy = runner.policies.at(0);
    auto foilState = runner.mdp->states[0];
    auto actionIndex = getActionIdsByLabels(runner, foilState->id, actions);

    //ASSERT_EQ(factPolicy->policy[0], actions[0]) << "After initial search, the only dominant policy should have picked action A.";


    // Lock state 0 to action A (with preceding policy pi.)
    //runner.solver->lockAction(*foilState, (ushort)actionIndex[0], *factPolicy);
    runner.explain(foilState->id, "B", factPolicy);
    auto policyIds = getPolicyIdsByStateAction(runner, 0, actions);
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(policyIds[0]), 0) << "After explaining, action 'A' (fact) policy should still have 0 non-acceptability.";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(policyIds[1]), 1) << "After explaining, action 'B' (foil) policy should now have 1 non-acceptability.";

}


TEST_F(ExplainTest, DepthTwoSearch) {
    Log::setLogLevel(Trace);
    Runner runner = Runner("DepthTwoSearch.json");
    vector<string> actions = {"A", "B"};
    runner.solve();
    ASSERT_EQ(runner.policies.size(), 2) << "After initial search, should be two dominant policies";


    // Check initial 'fact' policies.
    auto policyIds = getPolicyIdsByStateAction(runner, 0, actions);
    ASSERT_EQ(policyIds[0], -1) << "Should not exist a base fact policy with state 0 -> action A";

    policyIds = getPolicyIdsByStateAction(runner, 3, actions);
    ASSERT_NE(policyIds[0], -1) << "Should exist a base fact policy with state 3 -> action A.";
    ASSERT_NE(policyIds[1], -1) << "Should exist a base fact policy with state 3 -> action B.";

    // Each policy should attack the other
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(0), 0.5) << "Each initial policy should have 0.5 non-acceptability.";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(1), 0.5) << "Each initial policy should have 0.5 non-acceptability.";

    // Explainability part begins!
    // Query what if action A on state 0?
    auto foilState = runner.mdp->states[0];
    auto factPolicy = runner.policies[0];
    runner.explain(foilState->id, "A", factPolicy);
    // TODO mistake here because I calculated these based on the QValue not the histories.
    // Make policies more interesting or check for bugs.
    ASSERT_EQ(runner.policies.size(), 3) << "After second search, should be three policies";
    ASSERT_EQ(runner.non_accept.size(), 3) << "After second search, should be three policies, thus three non-acceptabilities.";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(0), 0.5) << "After second search, original non-acceptability should still be 0.5";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(1), 0.5) << "After second search, original non-acceptability should still be 0.5";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(2), 2) << "After second search, new policy non-acceptability should still be 2--fully attacked by both policies";

    // Policy just worse than original selects action A on state 1.
    policyIds = getPolicyIdsByStateAction(runner, 0, actions);
    ASSERT_NE(policyIds[0], -1) << "Should exist a foil policy with state 0 -> action A.";
    auto secondFactPolicy = runner.policies[policyIds[0]];
    ASSERT_EQ(secondFactPolicy->getActionAsString(*runner.mdp, 2), "B") << "Foil policy with 0->A should have state 2 -> action B.";

    // Query what if action A on state 2?
    foilState = runner.mdp->states[2];
    runner.explain(foilState->id, "A", secondFactPolicy);
    ASSERT_EQ(runner.policies.size(), 4) << "After third search, should be four policies";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(0), 0.5) << "After third search, original non-acceptability should still be 0.5";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(1), 0.5) << "After third search, original non-acceptability should still be 0.5";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(2), 2) << "After third search, first foil should still be 2--fully attacked by both theories";
    ASSERT_EQ(runner.non_accept.getPolicyNonAccept(3), 2) << "After third search, new (second) foil should be 2--fully attacked by both theories";


    policyIds = getPolicyIdsByStateAction(runner, 2, actions);
    ASSERT_NE(policyIds[0], -1) << "Should exist a foil policy with state 2 -> action A.";
    auto thirdFactPolicy = runner.policies[policyIds[0]];
    ASSERT_EQ(thirdFactPolicy->getActionAsString(*runner.mdp, 0), "A") << "New foil policy with 2->B should also have 0->A.";
}

