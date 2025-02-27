//
// Created by e56834sk on 23/08/2024.
//
#include "TestBase.hpp"
#include "MDP.hpp"
#include "Utilitarianism.hpp"
#include "Solution.hpp"

class QValueTestFixture : public TestBase {
protected:
    static void clearAndDeleteQValue(QValue &qv) {
        for (auto wb : qv.expectations) {
            delete wb;
        }
        qv.expectations.clear();
    }
    static void duoUtilityQValueCompare(int u1, int u2, int u3, int u4, int expectedResult, MDP *mdp, bool useRanks=false) {
        auto qv1 = QValue();
        qv1.expectations = {new ExpectedUtility(u1), new ExpectedUtility(u2)};
        auto qv2 = QValue();
        qv2.expectations = {new ExpectedUtility(u3), new ExpectedUtility(u4)};
        int actual = mdp->compareQValues(qv1, qv2, useRanks);
        EXPECT_EQ(actual, expectedResult)
            << "Comparison fail. (" << qv1.toString() << ") to (" << qv2.toString() << ") -> " << actual << " and not expected " << expectedResult;
        clearAndDeleteQValue(qv1);
        clearAndDeleteQValue(qv2);
    }
    static void trioUtilityQValueCompare(int u1, int u2, int u3, int u4, int u5, int u6, int expectedResult, MDP *mdp, bool useRanks=false) {
        auto qv1 = QValue();
        qv1.expectations = {new ExpectedUtility(u1), new ExpectedUtility(u2), new ExpectedUtility(u3)};
        auto qv2 = QValue();
        qv2.expectations = {new ExpectedUtility(u4), new ExpectedUtility(u5), new ExpectedUtility(u6)};
        int actual = mdp->compareQValues(qv1, qv2, useRanks);
        EXPECT_EQ(actual, expectedResult)
            << "Comparison fail. (" << qv1.toString() << ") to (" << qv2.toString() << ") -> " << actual << " and not expected " << expectedResult;
        clearAndDeleteQValue(qv1);
        clearAndDeleteQValue(qv2);
    }
};

TEST_F(QValueTestFixture, UtilityCompare) {
    MDP *mdp = MakeMDP("my_test.json");

    // Equal Ranks
    duoUtilityQValueCompare(0, 0, 0, 0, 0, mdp);

    // First better than second. Attack.
    duoUtilityQValueCompare(5, 0, 0, 0, 1, mdp);
    duoUtilityQValueCompare(0, 5, 0, 0, 1, mdp);
    // Second better than first. Reverse.
    duoUtilityQValueCompare(0, 0, 5, 0, -1, mdp);
    duoUtilityQValueCompare(0, 0, 0, 5, -1, mdp);

    // qv2 better by second theory, but loses by first. Dilemma.
    duoUtilityQValueCompare(1, 3, 0, 10, 2, mdp);
    // Reverse of above.
    duoUtilityQValueCompare(3, 1, 10, 0, 2, mdp);

    //
    // Same as above (but useRanks flag should not change results)
    //
    duoUtilityQValueCompare(0, 0, 0, 0, 0, mdp, true);
    duoUtilityQValueCompare(5, 0, 0, 0, 1, mdp, true);
    duoUtilityQValueCompare(0, 5, 0, 0, 1, mdp, true);
    duoUtilityQValueCompare(0, 0, 5, 0, -1, mdp, true);
    duoUtilityQValueCompare(0, 0, 0, 5, -1, mdp, true);
    duoUtilityQValueCompare(1, 3, 0, 10, 2, mdp, true);
    duoUtilityQValueCompare(3, 1, 10, 0, 2, mdp, true);


    //
    // First theory less preferred (greater rank)
    //
    mdp->theories[0]->rank = 1;
    mdp->groupedTheoryIndices = {{1}, {0}};
    // All equal. Draw
    duoUtilityQValueCompare(0, 0, 0, 0, 0, mdp, true);

    // if first better than second, attack
    duoUtilityQValueCompare(5, 0, 0, 0, 1, mdp, true);
    duoUtilityQValueCompare(0, 5, 0, 0, 1, mdp, true);
    // If second better than first, reverse
    duoUtilityQValueCompare(0, 0, 5, 0, -1, mdp, true);
    duoUtilityQValueCompare(0, 0, 0, 5, -1, mdp, true);

    // qv2 better by second theory, but loses by first. Qv1 attacks.
    duoUtilityQValueCompare(1, 3, 0, 10, -1, mdp, true);
    // Reverse of above.
    duoUtilityQValueCompare(3, 1, 10, 0, 1, mdp, true);

    delete mdp;
}