//
// Created by Simon Kolker on 23/09/2024.
//
#include "TestBase.hpp"
#include "Solver.hpp"

class MEHR_Tests : public TestBase {
protected:
    MEHR* mehr = nullptr;
    MDP* mdp = nullptr;
    vector<Policy*>* policies = nullptr;
    double tolerance = 1e-9;
    vector<double>* findNon_Accept(string fn) {
        mdp = MakeMDP(fn);
        Solver solver = Solver(*mdp);
        solver.MOiLAO();
        policies = solver.getSolutions();
        auto eh = new ExtractHistories(*mdp);
        auto histories = eh->extract(*policies);
        auto polExpectations = new vector<QValue>();
        polExpectations->reserve(policies->size());
        for (auto *pi : *policies) {
            polExpectations->push_back(pi->worth.at(0));
        }
        std::cout << eh->ToString(*policies, *polExpectations, histories) << std::endl;
        mehr = new MEHR(*mdp);
        auto non_accept = mehr->findNonAccept(*polExpectations, histories);
        std::cout << mehr->ToString(*polExpectations,histories) << std::endl;

        delete eh;
        delete polExpectations;

        return non_accept;
    }

    vector<size_t> getPolicyIdsByStateAction(int state_idx, vector<string> &actions) {
        auto stateActions = mdp->getActions(*mdp->states[state_idx]);
        // Map action string to Action Idx.
        vector<size_t> actionToIdx(actions.size(), -1);
        for (int i = 0; i < stateActions->size(); ++i) {
            for (size_t aIdx=0; aIdx<actions.size(); ++aIdx) {
                if (*stateActions->at(i)->label == actions[aIdx]) {
                    actionToIdx[aIdx] = i;
                }
            }
        }
        vector<size_t> actionToPolicyIdx(actions.size(), -1);
        for (int i=0; i < policies->size(); ++i) {
            for (size_t aIdx=0; aIdx<actionToIdx.size(); ++aIdx) {
                if (policies->at(i)->policy[state_idx] == actionToIdx[aIdx]) {
                    actionToPolicyIdx[aIdx] = i;
                }
            }
        }
        return actionToPolicyIdx;
    }

    void cleanup() {
        delete policies;
        delete mehr;
        delete mdp;
    }

};

TEST_F(MEHR_Tests, SimpleTest) {
    auto nonAccept = findNon_Accept("my_test.json");
    // Each policy should have 1 non-acceptability--attacks from both theories
    ASSERT_EQ((*nonAccept)[0], 1);
    ASSERT_EQ((*nonAccept)[1], 1);
    cleanup();
}


TEST_F(MEHR_Tests, LibraryTest_EqualRanks) {
    auto nonAccept = findNon_Accept("Library/EqualRanks.json");

    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(0, actions);

    ASSERT_NEAR((*nonAccept)[piIdx[0]], 1, tolerance);
    ASSERT_NEAR((*nonAccept)[piIdx[1]], 0.7, tolerance);

    cleanup();
}
TEST_F(MEHR_Tests, LibraryTest_No_Leaks_Priority) {
    auto nonAccept = findNon_Accept("Library/NoLeaksPriority.json");

    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(0, actions);

    ASSERT_NEAR((*nonAccept)[piIdx[0]], 1, tolerance);
    ASSERT_NEAR((*nonAccept)[piIdx[1]], 0, tolerance);

    cleanup();
}
TEST_F(MEHR_Tests, LibraryTest_Utility_Priority) {
    auto nonAccept = findNon_Accept("Library/UtilityPriority.json");

    vector<string> actions = {"Recommend", "Ignore"};
    auto piIdx = getPolicyIdsByStateAction(0, actions);

    ASSERT_NEAR((*nonAccept)[piIdx[0]], 0, tolerance);
    ASSERT_NEAR((*nonAccept)[piIdx[1]], 0.7, tolerance);

    cleanup();
}
