//
// Created by Simon Kolker on 20/01/2026.
//
#include "ExtractSolutions.hpp"
#include "TestBase.hpp"
#include "Logger.hpp"

class FromFile_Tests : public TestBase {
protected:
    double tolerance = 1e-9;
    json GetJSON(ifstream& file) {
        json data;
        try {
            data  = json::parse(file);
            file.close();
        }
        catch (json::parse_error& ex) {
            Log::writeLog(std::format("Error parsing MMMDP: {}", ex.what()), LogLevel::Error);
            file.close();
        }
        return data;
    }
    void TestAcceptability(std::string fileName) {
        auto f = Runner::OpenFile(fileName);
        auto data = GetJSON(f);
        MDP mdp(data);
        // Extract all 'policies'
        auto se = SolutionExtracter(mdp);
        vector<unique_ptr<Policy>> policies;
        vector<int> backupOrder = {1,0};
        vector<vector<int>> Pi(2);
        Pi[0].resize(mdp.getActions(*mdp.states[0])->size(),0);
        iota(Pi[0].begin(), Pi[0].end(), 0);
        se.Extract(policies, Pi, backupOrder);
        // Extract all 'policy' histories
        vector<vector<History*>> histories;
        auto eh = ExtractHistories(mdp);
        eh.extract(histories, policies);

        NonAcceptability nacc(mdp.mehr_theories.size(), policies.size());
        MEHR mehr(mdp, policies, histories);
        mehr.Slow_FindNonAccept(nacc);
        //std::cout << mehr.ToString(nacc) << std::endl;

        for (auto& soln : data["Solutions"]) {
            bool found = false;
            string action_name = soln["Action_Map"]["0"];
            for (size_t i=0; i < policies.size(); ++i) {
                Policy* pi = policies[i].get();
                auto actionIdx = pi->getAction(0);
                if (mdp.getActions(*mdp.states[0])->at(actionIdx)->label == action_name) {
                    // Contains a matching policy
                    found = true;
                    // Check policy has same non-acceptability
                    ASSERT_NEAR(nacc.getPolicyNonAccept(i), soln["Acceptability"], tolerance) << "Policy " << i << " following action " << action_name << " has wrong non-acceptability.";
                    // Check expectations are the same
                    auto myPiWorth = pi->getExpectationPtr();
                    for (size_t con_idx = 0; con_idx < myPiWorth->expectations.size(); ++con_idx) {
                        string con_label = mdp.considerations[con_idx]->label;
                        for (auto &expWorth : soln["Expectation"].items()) {
                            if (con_label != expWorth.key()) {
                                continue;
                            }
                            auto actual_util = dynamic_cast<ExpectedUtility*>(myPiWorth->expectations[con_idx].get());
                            ASSERT_NEAR(actual_util->value, expWorth.value(), tolerance) << "Mismatched expected utility " << " for consideration " << con_label << " on policy " << i << " with action " << action_name <<  ".";
                        }
                    }
                }
            }
            ASSERT_TRUE(found) << "Did not generate policy with action label " << action_name << endl;
        }
    }
};

TEST_F(FromFile_Tests, Acceptability) {
    // Small actions; mix
    TestAcceptability("Tests/test_2Theory_2Rank_3Actions.json");

    // 1 rank; small theories.
    TestAcceptability("Tests/test_1Theory.json");
    TestAcceptability("Tests/test_5Theory.json");
    TestAcceptability("Tests/test_10Theory.json");
    // small theories; 2 ranks
    TestAcceptability("Tests/test_5Theory_2Ranks.json");
    TestAcceptability("Tests/test_6Theory_2Ranks.json");
    TestAcceptability("Tests/test_7Theory_2Ranks.json");
    TestAcceptability("Tests/test_8Theory_2Ranks.json");
    TestAcceptability("Tests/test_9Theory_2Ranks.json");
    // 1 rank; many theories
    TestAcceptability("Tests/test_100Theory.json");
    TestAcceptability("Tests/test_200Theory.json");
    TestAcceptability("Tests/test_500Theory.json");
    TestAcceptability("Tests/test_1000Theory.json");
    // 100 theories; many ranks
    TestAcceptability("Tests/test_100Theory_25Rank.json");
    TestAcceptability("Tests/test_100Theory_50Rank.json");
    TestAcceptability("Tests/test_100Theory_75Rank.json");
    TestAcceptability("Tests/test_100Theory_100Rank.json");
}