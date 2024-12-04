from subprocess import call
import sys
import os
import pandas as pd
import numpy as np
from EnvironmentBuilder import MDPFactory
import json
from datetime import datetime

def addDictWithDefault(theDict, key, value, default=0):
    theDict.setdefault(key, default)
    theDict[key] += value
    


mdpFolder = os.getcwd() + "/MoralPlanner/Data/MDPs/"
outputFolder = os.getcwd() + "/MoralPlanner/Data/Experiments/" + datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "/"
os.makedirs(outputFolder, exist_ok=True)

planner = os.getcwd() + "/MoralPlanner/MPlan/cmake-build-release/MPlan"
domain = "LostInsulin"
REPETITIONS = 5
configs = {
    "HalCarlaEqual": ["0", "CarlaLife", "0", "HalLife"],
    "HalHigherCarla": ["0", "CarlaLife", "1", "HalLife"],
    "HalCarlaHigher": ["1", "CarlaLife", "0", "HalLife"],
    "HalCarlaNoSteal": ["0", "CarlaLife", "0", "HalLife", "0", "ToSteal"],
    "HalCarlaNoStealComp": ["1", "CarlaLife", "0", "HalLife", "0", "StealWithComp"],
    "CostCarlaSteal": ["0", "CarlaLife", "0", "Cost", "0", "ToSteal"],
    "CostCarla": ["0", "CarlaLife", "0", "Cost"]
}


# Call Env-Builder and build all environments.
for name, theories in configs.items():
    MDPFactory.buildEnvToFile(domain, theories, mdpFolder + name + ".json")
    print("Built: " + name)

print("Built all Environments")


# Call Planner on all environments.
for name, theories in configs.items():
    for rep in range(REPETITIONS):
        #eb.buildEnvToFile(domain, theories, outputFolder + name + ".json")
        call([planner, mdpFolder + name + ".json", outputFolder + name + "_" + str(rep) + ".json"])
    print("Ran MPlan on " + name + str(REPETITIONS) + " times.")


# Create data frame to populate
theoryKeys = []
# Get the moral theory tags and add it to the list of columns.
for name, theories in configs.items():
    with open(outputFolder + name + "_" + str(rep) + ".json", 'r') as file:
        json_data = json.load(file)
        for t in json_data["theories"]:
            if (not (t["Name"] in theoryKeys)):
                theoryKeys.append(t["Name"])
        

# Process each iteration
allRecords = [] # Records all data (even duplicate reps)
finalData = [] # Takes averages from duplicate reps.
recNum = 0
for name, theories in configs.items():
    # 1.
    # get info for all 5 repetitions of the experiment.
    for rep in range(REPETITIONS):
        allRecords.append({})
        with open(outputFolder + name + "_" + str(rep) + ".json", 'r') as file:
            json_data = json.load(file)
            allRecords[recNum]["avg. total time"] = json_data["duration_total"]
            allRecords[recNum]["avg. plan time"] = json_data["duration_Plan"]
            allRecords[recNum]["avg. MEHR time"] = json_data["duration_MEHR"]
            allRecords[recNum]["avg. expanded states"] = json_data["Expanded"]
            allRecords[recNum]["avg. backups"] = json_data["Backups"]
            allRecords[recNum]["Num of Candidates"] = json_data["SolutionTotal"]
            allRecords[recNum]["Min. non-acceptability"] = json_data["solutions"][0]["Non-Acceptability"]
            non_accepts = [json_data["solutions"][sol_idx]["Non-Acceptability"] for sol_idx in range(len(json_data["solutions"])) ]
            filtered_non_accepts = filter(lambda non_accept : non_accept==allRecords[recNum]["Min. non-acceptability"], non_accepts)
            allRecords[recNum]["Num of Cands w/ Min non-acceptability"] = len(list(filtered_non_accepts))
            allRecords[recNum]["config."] = str(name)
            for k in theoryKeys:
                if k in json_data["solutions"][0]["Expectation"].keys():
                    allRecords[recNum][k] = json_data["solutions"][0]["Expectation"][k]
                else:
                   allRecords[recNum][k] = "N/A"
            recNum+=1

    # 2.
    # Reduce to averages
    # PROBS GET RID OF THIS??
    record = {}
    for rep in range(REPETITIONS):
        addDictWithDefault(record, "avg. total time", (float(allRecords[rep]["avg. total time"]) * (1/5)), 0)
        addDictWithDefault(record, "avg. plan time", (float(allRecords[rep]["avg. plan time"]) * (1/5)), 0)
        addDictWithDefault(record, "avg. MEHR time", (float(allRecords[rep]["avg. MEHR time"]) * (1/5)), 0)

        addDictWithDefault(record, "avg. expanded states", (float(allRecords[rep]["avg. expanded states"]) * (1/5)), 0)
        addDictWithDefault(record, "avg. backups", (float(allRecords[rep]["avg. backups"]) * (1/5)), 0)
        addDictWithDefault(record, "Num of Candidates", (float(allRecords[rep]["Num of Candidates"]) * (1/5)), 0)

        addDictWithDefault(record, "Num of Candidates", (float(allRecords[rep]["Num of Candidates"]) * (1/5)), 0)
        addDictWithDefault(record, "Min. non-acceptability", (float(allRecords[rep]["Min. non-acceptability"]) * (1/5)), 0)

        record["config."] = str(theories)

        
df = pd.DataFrame(allRecords)
df.to_csv(outputFolder + "experiments_out.csv")

grouped_df = df.groupby("config.").mean(numeric_only=True)
grouped_df["Min. non-acceptability"] = np.round(grouped_df["Min. non-acceptability"], 5)
grouped_df.to_csv(outputFolder + "grouped_experiments_out.csv")
