//
// Created by Simon Kolker on 07/05/2025.
//
#include "TestBase.hpp"
#include "Utilitarianism.hpp"
#include "Logger.hpp"

class MaximinTestFixture : public TestBase {
protected:
    double tolerance = 1e-9;
};

TEST_F(MaximinTestFixture, OneOutcome) {
    Runner runner = Runner("Maximin_Tests/OneDepth_OneTheory_OneOutcome.json");
    //Log::setLogLevel(LogLevel::Trace);
    runner.FullSolve();

    vector<string> actions = {"A", "B"};
    auto piIdx = getPolicyIdsByStateAction(runner, 0, actions);

    // Action A wins.
    auto x = runner.non_accept->getPolicyNonAccept(piIdx["A"]);
    ASSERT_NEAR(x, 0, tolerance)
        << "Action A should have no attacks with its outcome worth (5,4) which beats (100,3), but it is attacked with "
        << runner.non_accept->getPolicyNonAccept(piIdx["A"])
        << " non-acceptability.";
    ASSERT_NEAR(runner.non_accept->getPolicyNonAccept(piIdx["B"]), 1, tolerance)
        << "Action B should be attacked on its only outcome worth (100,3) which beats (5,4), but it only has "
        << runner.non_accept->getPolicyNonAccept(piIdx["B"])
        << " non-acceptability.";
}

TEST_F(MaximinTestFixture, Bandit) {
    Runner runner = Runner("Maximin_Tests/OneDepth_OneTheory_ThreeOutcomes.json");
    runner.FullSolve();

    vector<string> actions = {"A", "B"};
    auto piIdx = getPolicyIdsByStateAction(runner, 0, actions);

    // Action A wins.
    ASSERT_NEAR(runner.non_accept->getPolicyNonAccept(piIdx["A"]), 0.3, tolerance)
        << "Action A should have one attack on [P=0.3, s'=1, -3, 0], causing 0.3 non-acceptability, but it has "
        << runner.non_accept->getPolicyNonAccept(piIdx["A"])
        << " non-acceptability.";
    ASSERT_NEAR(runner.non_accept->getPolicyNonAccept(piIdx["B"]), 0, tolerance)
        << "Action B should have no incoming attacks, but it only has "
        << runner.non_accept->getPolicyNonAccept(piIdx["B"])
        << " non-acceptability.";
}